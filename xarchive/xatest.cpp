#include <stdio.h>
#include <stdlib.h>

#include "glstreamer.h"

using namespace grinliz;

void MapData( StreamWriter* writer, int i )
{
	writer->OpenElement( "Data" );
	writer->CloseElement();
}


void MetaData( StreamWriter* writer )
{
	writer->OpenElement( "Meta" );
	writer->CloseElement();
}


void Map( StreamWriter* writer )
{
	writer->OpenElement( "Map" );
	for( int i=0; i<4; ++i ) {
		MapData( writer, i );
	}
	writer->CloseElement();
}


int main( int argc, const char* argv[] ) 
{
	printf( "Xarchive test.\n" );

	{
		FILE* fp = 0;
		fopen_s( &fp, "test.dat", "wb" );

		StreamWriter writer( fp );
		writer.OpenElement( "root" );
		Map( &writer );
		MetaData( &writer );
		writer.CloseElement();

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
