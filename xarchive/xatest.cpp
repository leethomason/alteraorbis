#include <stdio.h>
#include <stdlib.h>

#include "glstreamer.h"
#include "squisher.h"

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
		int vInt[] = { -20, 20 };
		float vFloat[] = { -20.2f, 20.2f };
		double vDouble[] = { -20.4, 20.4 };
		const char* meta[] = { "foo", "bar" };

		xs->Saving()->SetArr( "size-i", vInt, 2);
		xs->Saving()->SetArr( "size-f", vFloat, 2);
		xs->Saving()->SetArr( "size-d", vDouble, 2);
		xs->Saving()->SetArr( "meta", meta, 2 );
	}

	/*
	int nData = 4;
	XARC_SER( xs, nData );
	for( int i=0; i<nData; ++i ) {
		MapData( xs, i );
	}
	*/
	XarcClose( xs );
}


void KeyAttributes( XStream* xs )
{
	if ( xs->Saving() ) {
		static const int pos[13] = { 0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 1023, 1024, 10000 };
		static const int neg[12] = { -1, -2, -3, -7, -8, -9, -15, -16, -17, -1023, -1024, -10000 };
		xs->Saving()->SetArr( "positive", pos, 13 );
		xs->Saving()->SetArr( "negative", neg, 12 );
	}
}


int main( int argc, const char* argv[] ) 
{
	printf( "Xarchive test.\n" );


	if ( argc == 1 ) {
		{
			FILE* fp = 0;
			fopen_s( &fp, "test.dat", "wb" );

			StreamWriter writer( fp );
			writer.OpenElement( "root" );
			KeyAttributes( &writer );
			Map( &writer );
			//MetaData( &writer );
			writer.CloseElement();

			fclose( fp );
		}
		{
			FILE* fp = 0;
			fopen_s( &fp, "test.dat", "rb" );

			StreamReader reader( fp );
			reader.OpenElement();
			Map( &reader );
			//MetaData( &reader );
			reader.CloseElement();
	
			fclose( fp );
		}
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
