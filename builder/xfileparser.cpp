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


void XAnimationParser::Parse( const char* filename, gamedb::WItem* witem )
{
	GLOUTPUT(( "Parsing %s\n", filename ));
	
	FILE* fp = fopen( filename, "r" );
	GLASSERT( fp );
	GLString str;

	// Read the lines, but them in the main buffer.
	char buf[256];
	while ( GetLine( fp, buf, 256 )) {
		str.append( buf );
	}
	fclose( fp );



}

