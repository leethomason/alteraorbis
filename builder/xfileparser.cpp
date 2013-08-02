#include "xfileparser.h"

XAnimationParser::XAnimationParser()
{
}


XAnimationParser::~XAnimationParser()
{
}


void XAnimationParser::Parse( const char* filename, gamedb::WItem* witem )
{
	GLOUTPUT(( "Parsing %s\n", filename ));
	
	FILE* fp = fopen( filename, "r" );
	GLASSERT( fp );

	char buffer[256];
	while ( fgets
		

}

