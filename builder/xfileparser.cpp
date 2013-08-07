#include "xfileparser.h"
#include <string.h>

using namespace grinliz;



XAnimationParser::XAnimationParser()
{
	root = new Node();
	frameSkip = 0;
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


const char* XAnimationParser::ParseJoint( const char* p, BNode* node )
{
	p = SkipWhiteSpace( p );
	GLString name;
	while( *p && IsIdent(*p) ) {
		name.append( p, 1 );
		++p;
	}
	node->name = name;

	p = SkipWhiteSpace( p );
	GLASSERT( *p == '{' );
	++p;

	while( true ) {
		p = SkipWhiteSpace( p );
		if ( memcmp( p, "OFFSET", strlen( "OFFSET" ) ) == 0) {
			p += strlen( "OFFSET" );
			p = ScanFloat( &node->pos.x, p );
			p = SkipWhiteSpace( p );
			p = ScanFloat( &node->pos.y, p );
			p = SkipWhiteSpace( p );
			p = ScanFloat( &node->pos.z, p );
			p = SkipWhiteSpace( p );
		}
		else if ( memcmp( p, "CHANNELS", strlen( "CHANNELS" ) ) == 0) {
			p += strlen( "CHANNELS" );
			p = SkipWhiteSpace( p );
			int nChannels = *p - '0';
			++p;

			for( int ch=0; ch<nChannels; ++ch ) {
				p = SkipWhiteSpace( p );

				int select = 0;
				if ( *p == 'X' ) select = 0;
				else if ( *p == 'Y' ) select = 1;
				else if ( *p == 'Z' ) select = 2;
				else GLASSERT( 0 );

				++p;
				if ( memcmp( p, "position", strlen( "position" ) )) {
					p += strlen( "position" );
				}
				else if ( memcmp( p, "rotation", strlen( "rotation" ))) {
					p += strlen( "rotation" );
					select +=3;
				}
				else {
					GLASSERT( 0 );
				}

				Channel c = { node, select};
				channelArr.Push( c );

			}
		}
		else if ( memcmp( p, "End Site", strlen( "End Site" ) ) == 0 ) {
			// Safe to skip over, I think.
			while ( *p != '}' )
				++p;
			++p;
		}
		else if ( memcmp( p, "JOINT", strlen("JOINT") ) == 0 ) {
			p += strlen( "JOINT" );
			BNode* n = new BNode();
			n->parent = node;
			node->childArr.Push( n );
			p = ParseJoint( p, n );
		}
		else if ( *p == '}' ) {
			return p + 1;
		}
		else {
			GLASSERT( 0 );
		}
	}
	return p;
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
	node->parent = parent;

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


void XAnimationParser::DumpBNode( BNode* node, int depth )
{
	for( int i=0; i<depth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %.1f,%.1f,%.1f\n", node->name.c_str(), node->pos.x, node->pos.y, node->pos.z ));
	
	for( int i=0; i<node->childArr.Size(); ++i ) {
		DumpBNode( node->childArr[i], depth+1 );
	}
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


void XAnimationParser::DumpAnimation( AnimationNode* an )
{
	GLOUTPUT(( "AN: %s nFrames=%d\n", an->reference.c_str(), an->nFrames ));
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


void XAnimationParser::ReadAnimation( Node* node, AnimationNode* anim )
{
	GLASSERT( node->ident == "Animation" );
	
	// is first: determine number of frames & skip
	// then read in name, values

	// 3 keys and a reference
	GLASSERT( node->childArr.Size() == 4 );
	anim->reference = node->childArr[0]->ident;		// reference

	Node* key0 = node->childArr[1];
	Node* key1 = node->childArr[2];
	Node* key2 = node->childArr[3];
	int nFrames = (int)key0->floatArr[1];

	if ( frameSkip == 0 ) {
		GLASSERT( nFrames > 0 );
		frameSkip = 1;
		while( nFrames / frameSkip > EL_MAX_ANIM_FRAMES )
			++frameSkip;
	}
	anim->nFrames = nFrames / frameSkip;

	GLASSERT( key0->floatArr[0] == 0 );	// rotation
	GLASSERT( key0->floatArr.Size() == 2 + nFrames*6 );
	int f = 0;
	for( int i=0; i<nFrames; i+=frameSkip, ++f ) {
		Quaternion q;
		q.x = key0->floatArr[4+i*6+0];
		q.y = key0->floatArr[4+i*6+1];
		q.z = key0->floatArr[4+i*6+2];
		q.w = key0->floatArr[4+i*6+3];
		anim->rotation[f] = q;
	}

	GLASSERT( key1->floatArr[0] == 1 ); // scale
	GLASSERT( key1->floatArr.Size() == 2 + nFrames*5 );
	f = 0;
	for( int i=0; i<nFrames; i+=frameSkip, ++f ) {
		Vector3F v;
		v.x = key1->floatArr[4+i*5+0];
		v.y = key1->floatArr[4+i*5+1];
		v.z = key1->floatArr[4+i*5+2];
		anim->scale[f] = v;
	}

	GLASSERT( key2->floatArr[0] == 2 ); // position
	GLASSERT( key2->floatArr.Size() == 2 + nFrames*5 );
	f = 0;
	for( int i=0; i<nFrames; i+=frameSkip, ++f ) {
		Vector3F v;
		v.x = key2->floatArr[4+i*5+0];
		v.y = key2->floatArr[4+i*5+1];
		v.z = key2->floatArr[4+i*5+2];
		anim->pos[f] = v;
	}
}


void XAnimationParser::Write( const GLString& type, gamedb::WItem* witem )
{
	GLASSERT( animationArr.Size() );
	gamedb::WItem* animationItem = witem->FetchChild( type.c_str() );

	int nFrames = (int)animationArr[0]->nFrames;
	int totalDuration = (int)((float)(frameSkip*nFrames*1000)/ticksPerSecond);
	animationItem->SetInt( "totalDuration", totalDuration );

	for( int i=0; i<nFrames; ++i ) {
		gamedb::WItem* frameItem = animationItem->FetchChild( i );
		frameItem->SetInt( "time", (int)((float)(i*frameSkip) * 1000.f / ticksPerSecond));

		for( int j=0; j<animationArr.Size(); ++j ) {
			AnimationNode* an = animationArr[j];
			const char* name = an->reference.c_str();
			
			// Pull off the Armature_
			name = strstr( name, "_" );
			if ( !name ) continue;		// hopefully won't need this Armiture root node.
			++name;

			// Convert the underscores back to .
			GLString nStr = name;
			for( unsigned k=0; k<nStr.size(); ++k ) {
				if ( nStr[k] == '_' )
					nStr[k] = '.';
			}

			gamedb::WItem* boneItem = frameItem->FetchChild( nStr.c_str() );
			boneItem->SetFloatArray( "rotation", &an->rotation[i].x, 4 );
			boneItem->SetFloatArray( "scale",    &an->scale[i].x,    3 );
			boneItem->SetFloatArray( "position", &an->pos[i].x,      3 );
		}
	}
}



GLString XAnimationParser::GetAnimationType( const char* filename )
{
	GLString type;
	// Find the name postfix.
	const char* end = strstr( filename, ".x" );
	if ( !end ) { 
		end = strstr( filename, ".bvh" );
	}
	GLASSERT( end );
	if ( !end ) return "stand";

	const char* p = end;
	GLASSERT( p );
	--p;
	while ( p > filename && *p != '_' ) {
		--p;
	}
	GLASSERT( p > filename );
	type.append( p+1, end-p-1 );
	return type;
}


void XAnimationParser::ReadFile( const char* filename )
{
	FILE* fp = fopen( filename, "r" );
	GLASSERT( fp );

	// Read the lines, put them in the main buffer.
	char buf[256];
	// Throw away header:
	GetLine( fp, buf, 256 );
	// Read file into buffer
	str = "";
	while ( GetLine( fp, buf, 256 )) {
		str.append( buf );
	}
	fclose( fp );
}


void XAnimationParser::ParseBVH( const char* filename, gamedb::WItem* witem )
{
	GLString type = GetAnimationType( filename );
	GLOUTPUT(( "Parsing BVH %s action=%s\n", filename, type.c_str() ));
	ReadFile( filename );

	const char* p = str.c_str();

	bNodeRoot = new BNode();

	p = SkipWhiteSpace( p );
	GLASSERT( memcmp( p, "ROOT", strlen("ROOT") ) == 0 );
	p += 4;
	p = SkipWhiteSpace( p );
	p = ParseJoint( p, bNodeRoot );
	DumpBNode( bNodeRoot, 0 );
}


void XAnimationParser::Parse( const char* filename, gamedb::WItem* witem )
{
	GLString type = GetAnimationType( filename );
	GLOUTPUT(( "Parsing X %s action=%s\n", filename, type.c_str() ));
	
	// --- Read in from the file -- //
	ReadFile( filename );

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

	// -- Animation nodes -- //
	for( int i=0; i<root->childArr.Size(); ++i ) {
		Node* node = root->childArr[i];
		if ( node->ident == "AnimationSet" ) {
			for( int j=0; j<node->childArr.Size(); ++j ) {
				AnimationNode* anim = new AnimationNode();
				ReadAnimation( node->childArr[j], anim );
				animationArr.Push( anim );
			}
		}
	}

	for( int i=0; i<animationArr.Size(); ++i ) {
		DumpAnimation( animationArr[i] );
	}

	// Get the timing:
	ticksPerSecond = 24.0f;
	for( int i=0; i<root->childArr.Size(); ++i ) {
		Node* node = root->childArr[i];
		if ( node->ident == "AnimTicksPerSecond" ) {
			ticksPerSecond = node->floatArr[0];
		}
	}

	// Finally write:
	Write( type, witem );

	return;
}

