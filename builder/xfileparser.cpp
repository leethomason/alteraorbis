#include "xfileparser.h"
#include <string.h>

using namespace grinliz;



XAnimationParser::XAnimationParser()
{
	root = new Node();
}


XAnimationParser::~XAnimationParser()
{
	delete root;
}


bool XAnimationParser::GetLine( FILE* fp, char* buf, int size )
{
	char* p = buf;
	const char* end = buf + size - 1;
	int c = 0;
	while( p < end ) {
		c = getc( fp );
		if ( c < 0 || c == '\n' )
			break;
		*p = c;
		++p;
	}
	*p = 0;

	// Remove comments:
	char* comment = strstr( buf, "#" );
	if ( comment ) {
		*comment = 0;
	}
	comment = strstr( buf, "//" );
	if ( comment ) {
		*comment = 0;
	}
	
	return c >= 0;
}


const char* XAnimationParser::ParseDataObject( const char* p, Node* parent )
{
	p = SkipWhiteSpace( p );
	if ( *p == '{' ) {
		// annoying mini-object reference thing.
		++p;
		p = ParseDataObject( p, parent );
		p = SkipWhiteSpace( p );
	}
	if ( !*p )
		return p;

	// Skip over template definitions.
	if ( memcmp( p, "template", 8 ) == 0 ) {
		// Skip ahead.
		while ( *p != '}' )
			++p;
		++p;
		p = SkipWhiteSpace( p );
	}

	GLString identifier, name;

	while( *p && IsIdent( *p )) {
		identifier.append( p, 1 );
		++p;
	}
	GLASSERT( !identifier.empty() );
	
	p = SkipWhiteSpace( p );
	if ( *p != '{' && *p != '}' ) { 
		while( *p && IsIdent( *p )) {
			name.append( p, 1 );
			++p;
		}
	}

	p = SkipWhiteSpace( p );
	GLASSERT( *p == 0 || *p == '{' || *p == '}' );

	if ( *p == 0 ) 
		return p;

	Node* node = new Node();
	parent->childArr.Push( node );
	node->ident = identifier;
	node->name  = name;

	if ( *p == '}' ) {
		++p;
		return p;
	}

	++p;
	p = SkipWhiteSpace( p );

	while( true ) {
		// Numbers; leaf node.
		if ( isdigit( *p ) || (*p=='-')) {

			while( true ) {
				float v = 0;
				p = SkipNumDelimiter( p );
				if ( IsNum( *p )) {
					p = ScanFloat( &v, p );
					node->floatArr.Push( v );
				}
				else {
					break;
				}
			}

			while ( *p != '}' )
				++p;
			GLASSERT( *p == '}' );
			++p;
			break;
		}
		else {
			// Sub-object
			p = ParseDataObject( p, node );
			p = SkipWhiteSpace( p );
			if ( *p == '}' ) {
				++p;
				break;	// closing object.
			}
		}
	}

	return p;
}


void XAnimationParser::DumpNode( Node* node, int depth )
{
	for( int i=0; i<depth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %s\n", node->ident.c_str(), node->name.c_str() ));
	
	for( int i=0; i<node->childArr.Size(); ++i ) {
		DumpNode( node->childArr[i], depth+1 );
	}
}


void XAnimationParser::DumpFrameNode( FrameNode* node, int depth )
{
	for( int i=0; i<depth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %s\n", node->ident.c_str(), node->name.c_str() ));
	node->xform.Dump( "xform" );
	
	for( int i=0; i<node->childArr.Size(); ++i ) {
		DumpFrameNode( node->childArr[i], depth+1 );
	}
}


const char* XAnimationParser::ScanFloat( float* v, const char* p )
{
	p = SkipWhiteSpace( p );
	if ( IsNum( *p ) ) {
		*v = (float)atof( p );
		++p;
		while( IsNum( *p )) {
			p++;
		}
	}
	return p;
}


void XAnimationParser::WalkFrameNodes( FrameNode* fn, Node* node )
{
	fn->ident = node->ident;
	fn->name = node->name;

	GLASSERT( node->childArr.Size() );
	Node* frameXForm = node->childArr[0];
	GLASSERT( frameXForm->ident == "FrameTransformMatrix" );
	GLASSERT( frameXForm->floatArr.Size() == 16 );
	for( int i=0; i<16; ++i ) {
		fn->xform.x[i] = frameXForm->floatArr[i];
	}
	fn->xform.Transpose();

	for( int i=0; i<node->childArr.Size(); ++i ) {
		Node* n = node->childArr[i];
		if ( n->ident == "Frame" ) {
			FrameNode* child = new FrameNode();
			child->parent = fn;
			fn->childArr.Push( child );
			WalkFrameNodes( child,n );
		}
	}
}


void XAnimationParser::Parse( const char* filename, gamedb::WItem* witem )
{
	GLOUTPUT(( "Parsing %s\n", filename ));
	
	// --- Read in from the file -- //
	FILE* fp = fopen( filename, "r" );
	GLASSERT( fp );

	// Read the lines, put them in the main buffer.
	char buf[256];
	// Throw away header:
	GetLine( fp, buf, 256 );
	// Read file into buffer
	while ( GetLine( fp, buf, 256 )) {
		str.append( buf );
	}
	fclose( fp );

	// -- Parse the data objects -- //
	const char* p = str.c_str();
	const char* end = p + str.size();

	while( p < end ) {
		p = ParseDataObject( p, root );
	}
	DumpNode( root, 0 );

	// -- Coordinate frames -- //
	for( int i=0; i<root->childArr.Size(); ++i ) {
		Node* node = root->childArr[i];
		if ( node->ident == "Frame" && node->name == "Root" ) {
			frameNodeRoot = new FrameNode();
			WalkFrameNodes( frameNodeRoot, node );
			break;
		}
	}
	DumpFrameNode( frameNodeRoot, 0 );

	return;
}

