#include <stdio.h>
#include <stdlib.h>

#include "glstreamer.h"

using namespace grinliz;

void MapData( XStream* xs, int i )
{
	XCStr str = "Data";
	xs->OpenElement( str );
	xs->CloseElement();
}


void MetaData( XStream* xs )
{
	XCStr str = "Meta";
	xs->OpenElement( str );
	xs->CloseElement();
}


void Map( XStream* xs )
{
	XCStr str = "Map";
	xs->OpenElement( str );
	for( int i=0; i<4; ++i ) {
		MapData( xs, i );
	}
	xs->CloseElement();
}


int main( int argc, const char* argv[] ) 
{
	printf( "Xarchive test.\n" );

	{
		FILE* fp = 0;
		fopen_s( &fp, "test.dat", "wb" );

		StreamWriter writer( fp );
		writer.OpenElement( XCStr("root") );
		Map( &writer );
		MetaData( &writer );
		writer.CloseElement();

		fclose( fp );
	}

	{
		FILE* fp = 0;
		fopen_s( &fp, "test.dat", "rb" );

		StreamReader reader( fp );
		reader.OpenElement( XCStr("root") );
		Map( &reader );
		MetaData( &reader );
		reader.CloseElement();
	
		fclose( fp );
	}


	{
		FILE* fp = 0;
		fopen_s( &fp, "test.dat", "rb" );

		StreamReader reader( fp );
		printf( "version=%d\n", reader.Version() );
		DumpStream( &reader, 0 );

		fclose( fp );
	}
	return 0;
}
