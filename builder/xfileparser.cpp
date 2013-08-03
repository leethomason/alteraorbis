#include "xfileparser.h"

using namespace grinliz;

XAnimationParser::XAnimationParser()
{
}


XAnimationParser::~XAnimationParser()
{
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
	if ( comment )
		*comment = 0;
	
	return c >= 0;
}


const char* XAnimationParser::ParseDataObject( const char* p, int depth )
{
	p = SkipWhiteSpace( p );
	if ( *p == '{' ) {
		// annoying mini-object
		++p;
		p = ParseDataObject( p, depth+1 );
		p = SkipWhiteSpace( p );
	}
	if ( !*p )
		return p;

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

	for( int i=0; i<depth; ++i ) {
		GLOUTPUT(( "  " ));
	}
	GLOUTPUT(( "  %s %s\n", identifier.c_str(), name.c_str() ));

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

	while( true ) {
		// Numbers; leaf node.
		if ( isdigit( *p ) || (*p=='-')) {
			while ( *p != '}' )
				++p;
			p = SkipWhiteSpace( p );
			GLASSERT( *p == '}' );
			++p;
			break;
		}
		else {
			p = ParseDataObject( p, depth+1 );
			p = SkipWhiteSpace( p );
			if ( *p == '}' ) {
				++p;
				break;	// closing object.
			}
		}
	}
	return p;
}


void XAnimationParser::Parse( const char* filename, gamedb::WItem* witem )
{
	GLOUTPUT(( "Parsing %s\n", filename ));
	
	FILE* fp = fopen( filename, "r" );
	GLASSERT( fp );

	// Read the lines, but them in the main buffer.
	char buf[256];
	while ( GetLine( fp, buf, 256 )) {
		str.append( buf );
	}
	fclose( fp );

	const char* p = str.c_str();
	const char* end = p + str.size();

	while( p < end ) {
		p = ParseDataObject( p, 0 );
	}

}

