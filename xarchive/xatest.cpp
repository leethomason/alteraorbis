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

	int     i = 1;
	float   j = 2;
	double  k = 3;
	IString s = StringPool::Intern( "istring" );

	XARC_SER( xs, j );
	XARC_SER( xs, i );
	XARC_SER( xs, k );
	XARC_SER( xs, s );

	// Asserts do nothing on save(). checks load() worked.
	GLASSERT( i == 1 );
	GLASSERT( j == 2 );
	GLASSERT( k == 3 );
	GLASSERT( s == "istring" );

	int     iArr[] = { 1, -1000 };
	float   jArr[] = { 2, -2000.0f };
	double  kArr[] = { 3, -3000.0 };
	IString sArr[] = { StringPool::Intern( "istring0" ), StringPool::Intern( "istring" ) };

	XARC_SER_ARR( xs, iArr, 2 );
	XARC_SER_ARR( xs, jArr, 2 );
	XARC_SER_ARR( xs, kArr, 2 );
	XARC_SER_ARR( xs, sArr, 2 );

	GLASSERT( iArr[1] == -1000 );
	GLASSERT( jArr[1] == -2000.0f );
	GLASSERT( kArr[1] == -3000.0 );
	GLASSERT( sArr[1] == "istring" );

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

	int nData = 4;
	XARC_SER( xs, nData );
	for( int i=0; i<nData; ++i ) {
		MapData( xs, i );
	}
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
