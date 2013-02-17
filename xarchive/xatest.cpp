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

	float x = 1;
	double y = 1;
	U8 z = 1;
	XARC_SER( xs, x );
	XARC_SER( xs, y );
	XARC_SER( xs, z );
	
	int iArr[2] = { 0, 1 };
	float fArr[2] = { 0, 1 };
	float dArr[2] = { 0, 1 };
	U8 bArr[2] = { 0, 1 };

	XARC_SER_ARR( xs, iArr, 2 );
	XARC_SER_ARR( xs, fArr, 2 );
	XARC_SER_ARR( xs, dArr, 2 );
	XARC_SER_ARR( xs, bArr, 2 );

	if ( xs->Saving() ) {
		xs->Saving()->Set( "foo0", "bar0" );
		xs->Saving()->Set( "foo1", "bar1" );
	}
	else {
		xs->Loading()->Get( "foo0" );
		xs->Loading()->Get( "foo1" );
	}
	
	XarcClose( xs );
}


void Map( XStream* xs )
{
	XarcOpen( xs, "Map" );

	if ( xs->Saving() ) {
		int b[] = { -1, -2, -3, -4 };
		xs->Saving()->SetArr( "bounds", b, 4 );
	}

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


	if ( argc == 1 ) {
		FILE* fp = 0;
		fopen_s( &fp, "test.dat", "wb" );

		StreamWriter writer( fp );
		writer.OpenElement( "root" );
		Map( &writer );
		MetaData( &writer );
		writer.CloseElement();

		fclose( fp );
	}

	if ( argc == 1 ) {
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
		const char* fname = "test.dat";
		if ( argc >= 2 ) {
			fname = argv[1];
		}
		fopen_s( &fp, fname, "rb" );

		StreamReader reader( fp );
		printf( "version=%d\n", reader.Version() );
		DumpStream( &reader, 0 );

		fclose( fp );
	}
	return 0;
}
