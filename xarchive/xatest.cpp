#include <stdio.h>
#include <stdlib.h>

#include "glstreamer.h"

using namespace grinliz;

void MapData( XStream* xs, int i )
{
	XarcOpen( xs, "Data" );

	int local=i;
	XARC_SER( xs, i );
	GLASSERT( local == i );

	XarcClose( xs );
}


void MetaData( XStream* xs )
{
	XarcOpen( xs, "Meta" );
	int i=1, j=2, k=3;
	XARC_SER( xs, j );
	XARC_SER( xs, i );
	XARC_SER( xs, k );
	XarcClose( xs );
}


void Map( XStream* xs )
{
	XarcOpen( xs, "Map" );
	int nData = 4;
	XARC_SER( xs, nData );
	for( int i=0; i<nData; ++i ) {
		MapData( xs, i );
	}
	XarcClose( xs );
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
		reader.OpenElement();
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
