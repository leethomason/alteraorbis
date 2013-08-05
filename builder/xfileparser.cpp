#include "xfileparser.h"
#include <string.h>

using namespace grinliz;


XAnimationParser::FrameNode::~FrameNode()
{
}


XAnimationParser::XAnimationParser()
{
	rootFrameNode = 0;
	scanDepth = 0;
}


XAnimationParser::~XAnimationParser()
{
	delete rootFrameNode;
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


const char* XAnimationParser::ParseDataObject( const char* p, int depth )
{
	p = SkipWhiteSpace( p );
	if ( *p == '{' ) {
		// annoying mini-object reference thing.
		++p;
		p = ParseDataObject( p, depth+1 );
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
	if ( *p == '}' ) {
		++p;
		return p;
	}

	++p;
	p = SkipWhiteSpace( p );

	Push( identifier, name );

	while( true ) {
		// Numbers; leaf node.
		if ( isdigit( *p ) || (*p=='-')) {

			Scan( identifier, p );
			while ( *p != '}' )
				++p;
			p = SkipWhiteSpace( p );
			GLASSERT( *p == '}' );
			++p;
			break;
		}
		else {
			// Sub-object
			p = ParseDataObject( p, depth+1 );
			p = SkipWhiteSpace( p );
			if ( *p == '}' ) {
				++p;
				break;	// closing object.
			}
		}
	}

	Pop( identifier, name );
	return p;
}

void XAnimationParser::Push( const grinliz::GLString& ident, const grinliz::GLString& name )
{
	if ( ident == "FrameTransformMatrix" ) {
		// This gets applied to the frame as an xform; it does NOT create a new frame.
		// Scan() does the work.
		return;
	}
	if ( ident != "Frame" ) {
		return;
	}

	FrameNode* fm = new FrameNode();
	if ( rootFrameNode ) {
		fm->parent = currentFrameNode;
		currentFrameNode->childArr.Push( fm );
	}
	else {
		rootFrameNode = fm;
	}
	fm->ident = ident;
	fm->name = name;

	for( int i=0; i<scanDepth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %s\n", name.c_str(), ident.c_str() ));
	
	++scanDepth;
	currentFrameNode = fm;
}


void XAnimationParser::Pop( const grinliz::GLString& ident, const grinliz::GLString& name )
{
	if ( ident == "FrameTransformMatrix" ) {
		return;
	}
	if ( ident != "Frame" ) {
		return;
	}

	currentFrameNode = currentFrameNode->parent;
	--scanDepth;
}


void XAnimationParser::ScanFloats( float* arr, int count, const char* p )
{
	int i=0;
	while( i<count ) {
		if ( IsNum( *p ) ) {
			arr[i] = (float)atof( p );
			++i;
			while( IsNum( *p )) {
				p++;
			}
		}
		else { 
			p++;
		}
	}
}


void XAnimationParser::Scan( const GLString& ident, const char* p )
{
	if ( ident == "FrameTransformMatrix" ) {
		ScanFloats( currentFrameNode->xform.x, 16, p );
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
		p = ParseDataObject( p, 0 );
	}

}

