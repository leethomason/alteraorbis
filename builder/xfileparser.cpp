#include "xfileparser.h"
#include <string.h>

using namespace grinliz;

/*
	Coordinate systems:

	OpenGL:

	+----x	|
	|		|
	|	  Front
	z

	Blender:

	y		|
	|		|
	|	  Front
	+----x


	Maybe this is how export works, to say nothing of rotation.
	glX = bX
	glY = bZ
	glZ = -bY
*/


XAnimationParser::XAnimationParser()
{
	frameSkip = 0;
}


XAnimationParser::~XAnimationParser()
{
	delete bNodeRoot;
}


bool XAnimationParser::GetLine( FILE* fp, GLString* str )
{
	*str = "";

	int c = 0;
	while( true ) {
		c = getc( fp );
		if ( c < 0 )
			break;
		if ( c == '\n' ) {
			str->append( " " );
			break;
		}
		char b = (char)c;
		str->append( &b, 1 );
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
			float x, y, z;
			p = ScanFloat( &x, p );
			p = ScanFloat( &y, p );
			p = ScanFloat( &z, p );
			node->refPos.Set( x, z, -y );	// swizzle for coordinate xform
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
				if ( memcmp( p, "position", strlen( "position" )) == 0 ) {
					p += strlen( "position" );
				}
				else if ( memcmp( p, "rotation", strlen( "rotation" )) == 0) {
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


const char* XAnimationParser::ParseMotion( const char* p )
{
	p = SkipWhiteSpace( p );
	GLASSERT( memcmp( p, "MOTION", strlen( "MOTION" )) == 0 );
	p += strlen( "MOTION" );

	p = SkipWhiteSpace( p );
	GLASSERT( memcmp( p, "Frames:", strlen( "Frames:" )) == 0 );
	p += strlen( "Frames:" );
	p = SkipWhiteSpace( p );
	
	float nFramesF = 0;
	p = ScanFloat( &nFramesF, p );
	nFrames = (int)nFramesF;
	GLASSERT( nFrames );

	p = SkipWhiteSpace( p );
	GLASSERT( memcmp( p, "Frame Time:", strlen( "Frame Time:" )) == 0 );
	p += strlen( "Frame Time:" );
	p = SkipWhiteSpace( p );
	p = ScanFloat( &frameTime, p );

	frameSkip = 1;
	while ( nFrames / frameSkip > EL_MAX_ANIM_FRAMES ) {
		++frameSkip;
	}
	frameTime *= (float)frameSkip;
	int nFramesInFile = nFrames;
	nFrames /= frameSkip;

	int nChannels = channelArr.Size();
	int skip = 0;
	int frame = 0;

	for( int i=0; i<nFramesInFile; ++i ) {
		for( int j=0; j<nChannels; ++j ) {
			p = SkipWhiteSpace( p );
			float v = 0;
			p = ScanFloat( &v, p );
			if ( skip == 0 ) {
				int select = channelArr[j].select;
				BNode* n = channelArr[j].node;
				n->channel[frame][select] = v;

				if ( select < 3 ) {
					GLASSERT( v >= -4 && v < 4 );
				}
				else {
					GLASSERT( v >= -360 && v <= 360 );
				}
			}
		}
		++skip;
		if ( skip == frameSkip ) {
			skip = 0;
			++frame;
		}
	}
	p = SkipWhiteSpace( p );
	GLASSERT( *p == 0 );
	return p;
}


void XAnimationParser::DumpBNode( BNode* node, int depth )
{
	for( int i=0; i<depth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %.1f,%.1f,%.1f\n", node->name.c_str(), node->refPos.x, node->refPos.y, node->refPos.z ));
	
	for( int i=0; i<node->childArr.Size(); ++i ) {
		DumpBNode( node->childArr[i], depth+1 );
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


void XAnimationParser::WriteBVHRec( gamedb::WItem* frameItem, int n, BNode* node )
{
	gamedb::WItem* boneItem = frameItem->FetchChild( node->name.c_str() );

	/* swizzle for coordinate xform
	glX = bX
	glY = bZ
	glZ = -bY
	*/
	Vector3F pos = {	node->channel[n][0],
						node->channel[n][2],
						-node->channel[n][1] };

	float xr = node->channel[n][3];
	float yr = node->channel[n][5];
	float zr = -node->channel[n][4];

	Vector3F euler = { ToRadian(xr), ToRadian(yr), ToRadian(zr) };

	// XYZs took a while to figure out.
	Quaternion q = EulerToQuat( euler, EulOrdXYZs );

	if ( node->parent ) {
		boneItem->SetString( "parent", node->parent->name.c_str() );
	}
	boneItem->SetFloatArray( "rotation", &q.x, 4 );

	Vector3F refPos = node->refPos;
	boneItem->SetFloatArray( "position", &pos.x, 3 );
	boneItem->SetFloatArray( "refPosition", &refPos.x, 3 );

	if ( node->childArr.Size() ) {
		gamedb::WItem* subItem = frameItem->FetchChild( node->name.c_str() );
		for( int i=0; i<node->childArr.Size(); ++i ) {
			WriteBVHRec( subItem, n, node->childArr[i] );
		}
	}
}


void XAnimationParser::WriteBVH( gamedb::WItem* witem )
{
	GLASSERT( bNodeRoot );
	gamedb::WItem* animationItem = witem->FetchChild( typeName.c_str() );

	float totalDurationF = (float)nFrames * frameTime;
	int totalDuration = (int)(totalDurationF * 1000.0f);
	animationItem->SetInt( "totalDuration", totalDuration );

	for( int i=0; i<nFrames; ++i ) {
		gamedb::WItem* frameItem = animationItem->FetchChild( i );
		frameItem->SetInt( "time", totalDuration*i/nFrames );
		WriteBVHRec( frameItem, i, bNodeRoot );
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
	GLString buf;
	// Throw away header:
	GetLine( fp, &buf );
	// Read file into buffer
	str = "";
	while ( GetLine( fp, &buf )) {
		str.append( buf );
	}
	fclose( fp );
}


void XAnimationParser::ParseBVH( const char* filename, gamedb::WItem* witem )
{
	typeName = GetAnimationType( filename );
	GLOUTPUT(( "Parsing BVH %s action=%s\n", filename, typeName.c_str() ));
	ReadFile( filename );

	const char* p = str.c_str();

	bNodeRoot = new BNode();

	p = SkipWhiteSpace( p );
	GLASSERT( memcmp( p, "ROOT", strlen("ROOT") ) == 0 );
	p += 4;
	p = SkipWhiteSpace( p );
	p = ParseJoint( p, bNodeRoot );
	//DumpBNode( bNodeRoot, 0 );

	ParseMotion( p );

	witem->SetInt( "version", 2 );
	WriteBVH( witem );
}
