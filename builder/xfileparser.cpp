#include "xfileparser.h"
#include <string.h>

using namespace grinliz;



XAnimationParser::XAnimationParser()
{
	frameSkip = 0;
}


XAnimationParser::~XAnimationParser()
{
	delete bNodeRoot;
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

	for( int i=0; i<nFramesInFile; ++i, ++skip ) {
		for( int j=0; j<nChannels; ++j ) {
			p = SkipWhiteSpace( p );
			float v = 0;
			ScanFloat( &v, p );
			if ( skip == 0 ) {
				channelArr[j].node->channel[frame][channelArr[j].select] = v;
			}
		}
		++skip;
		if ( skip == frameSkip ) {
			skip = 0;
			++frame;
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

	Vector3F pos = {	node->channel[n][0],
						node->channel[n][1],
						node->channel[n][2] };

	Matrix4 xRot, yRot, zRot;
	xRot.SetXRotation( node->channel[n][3] );
	xRot.SetYRotation( node->channel[n][4] );
	xRot.SetZRotation( node->channel[n][5] );

	Matrix4 rot = xRot * yRot * zRot;	// Order?
	Quaternion q;
	q.FromRotationMatrix( rot );

	if ( node->parent ) {
		boneItem->SetString( "parent", node->parent->name.c_str() );
	}
	boneItem->SetFloatArray( "rotation", &q.x, 4 );
	boneItem->SetFloatArray( "position", &pos.x, 3 );
}


void XAnimationParser::WriteBVH( const grinliz::GLString& type, gamedb::WItem* witem )
{
	GLASSERT( bNodeRoot );
	gamedb::WItem* animationItem = witem->FetchChild( type.c_str() );

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

	ParseMotion( p );

	WriteBVH( type, witem );
}
