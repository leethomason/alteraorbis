#include <stdio.h>
#include <stdlib.h>

#include "glstreamer.h"
#include "../grinliz/glmicrodb.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glrandom.h"
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
	Matrix4 iMat, tMat;
	tMat.SetTranslation( 2, 2, 2 );

	XARC_SER( xs, j );
	XARC_SER( xs, i );
	XARC_SER( xs, k );
	XARC_SER( xs, s );
	XARC_SER( xs, iMat );
	XARC_SER( xs, tMat );

	// Asserts do nothing on save(). checks load() worked.
	GLASSERT( i == 1 );
	GLASSERT( j == 2 );
	GLASSERT( k == 3 );
	GLASSERT( s == "istring" );
	GLASSERT( iMat.IsIdentity() );
	GLASSERT( tMat.X(Matrix4::M14) == 2.0f );

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
		float vFloat[] = { -20.2f, 20.2f, 0.5f, 1.5f, -0.5f, 1.5f, 10.0f, -20.0f };
		double vDouble[] = { -20.4, 20.4, 20.125 };
		const char* meta[] = { "foo", "bar" };

		xs->Saving()->SetArr( "size-i", vInt, 2);
		xs->Saving()->SetArr( "size-f", vFloat, 8);
		xs->Saving()->SetArr( "size-d", vDouble, 3);
		xs->Saving()->SetArr( "meta", meta, 2 );
	}

	int nData = 4;
	XARC_SER( xs, nData );
	for( int i=0; i<nData; ++i ) {
		MapData( xs, i );
	}
	XarcClose( xs );
}


void Limits( XStream* xs ) 
{
	XarcOpen( xs, "limits" );

	int maxint = INT_MAX;
	int minint = INT_MIN;
	XARC_SER( xs, maxint );
	XARC_SER( xs, minint );
	GLASSERT( maxint == INT_MAX );
	GLASSERT( minint == INT_MIN );

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


void FloatEncoding(XStream* xs) 
{
	float oneF = 1;
	float zeroF = 0;
	float point5F = 1023.0f + 0.5f;
	float intF = 1023;
	float valueF = 1.23456f;

	double oneD = 1;
	double zeroD = 0;
	double point5D = 1023.0 + 0.5;
	double intD = 1023;
	double valueD = 1.23456;

	XarcOpen(xs, "FloatEncoding");
	XARC_SER(xs, oneF);
	XARC_SER(xs, zeroF);
	XARC_SER(xs, point5F);
	XARC_SER(xs, intF);
	XARC_SER(xs, valueF);

	XARC_SER(xs, oneD);
	XARC_SER(xs, zeroD);
	XARC_SER(xs, point5D);
	XARC_SER(xs, intD);
	XARC_SER(xs, valueD);
	XarcClose(xs);

	GLASSERT(oneF == 1.0f);
	GLASSERT(zeroF == 0.0f);
	GLASSERT(point5F = 1023.5f);
	GLASSERT(intF == 1023);
	GLASSERT(valueF == 1.23456f);

	GLASSERT(oneD == 1.0);
	GLASSERT(zeroD == 0.0);
	GLASSERT(point5D = 1023.5);
	GLASSERT(intD == 1023);
	GLASSERT(valueD == 1.23456);
}


//inline bool GL_FUNC_ASSERT(bool x) { GLASSERT(x); return x; }

int main( int argc, const char* argv[] ) 
{
	printf( "Xarchive test.\n" );

	if ( argc == 1 ) {
		{
			FILE* fp = 0;
			fopen_s( &fp, "test.dat", "wb" );

			StreamWriter writer( fp, 100 );
			writer.OpenElement( "root" );
			KeyAttributes( &writer );
			Map( &writer );
			MetaData( &writer );
			Limits( &writer );
			FloatEncoding(&writer);
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
			Limits( &reader );
			FloatEncoding(&reader);
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

	// Tacked on DB testing:
	printf( "----- MicroDB -----\n" );
	{
		MicroDB microdb;

		microdb.Set( "test-i", StringPool::Intern( "valueI" ) );
		microdb.Set( "test-d", -3 );
		microdb.Set( "3val", 42.13f );

		IString s;
		int i;

		microdb.Get( "test-i", &s );
		printf( "Get: (valueI) %s\n", s.c_str() );

		microdb.Get( "test-d", &i );
		printf( "Get: (-3) %d\n", i );

		FILE* fp = 0;
		fopen_s( &fp, "microdbtest.dat", "wb" );
		StreamWriter writer( fp, 100 );
		microdb.Serialize( &writer, "test" );
		fclose( fp ); 
	}
	{
		FILE* fp = 0;
		fopen_s( &fp, "microdbtest.dat", "rb" );
		StreamReader reader( fp );
		DumpStream( &reader, 0 );
		fclose( fp );
	}
	{
		FILE* fp = 0;
		fopen_s(&fp, "microdbtest.dat", "rb");
		if (fp) {
			StreamReader reader(fp);
			MicroDB microdb;
			microdb.Serialize(&reader, "test");

			float f;
			microdb.Get("3val", &f);
			printf("Get: %f\n", f);

			fclose(fp);
		}
	}
	{
		{
			FILE* fp = 0;
			fopen_s( &fp, "squishtest.dat", "wb" );
			if (fp) {
				Squisher squisher;
				static const char* test0 = "Hello world. This is compressed data.";
				static const char* test1 = "This is a 2nd line of comrpessed data, and concludes the test.";

				int len = strlen(test0) + 1;
				squisher.StreamEncode(&len, 4, fp);
				squisher.StreamEncode(test0, strlen(test0) + 1, fp);
				len = strlen(test1) + 1;
				squisher.StreamEncode(&len, 4, fp);
				squisher.StreamEncode(test1, strlen(test1) + 1, fp);
				squisher.StreamEncode(0, 0, fp);

				printf("Encode: Unc=%d Comp=%d\n", squisher.encodedU, squisher.encodedC);

				fclose(fp);
			}
		}
		{
			FILE* fp = 0;
			fopen_s( &fp, "squishtest.dat", "rb" );
			Squisher squisher;
			char buffer[200];

			int len=0;
			squisher.StreamDecode( &len, 4, fp );
			squisher.StreamDecode( buffer, len, fp );
			printf( "%s\n", buffer );
			squisher.StreamDecode( &len, 4, fp );
			squisher.StreamDecode( buffer, len, fp );
			printf( "%s\n", buffer );

			printf( "Decode: Unc=%d Comp=%d\n", squisher.decodedU, squisher.decodedC );
		}

	}

	printf("Random test.\n");
	{
		Random random;
		static const int NBUCKETS = 4;
		static const int DIV = UINT32_MAX / 4;
		int bucket[NBUCKETS] = { 0 };
		int fbucket[NBUCKETS] = { 0 };

		for (int i = 0; i < 10000; ++i) {
			bucket[random.Rand() / DIV]++;

			int f = int(random.Uniform() * 4.0f);
			if (f == 4) f = 3;
			fbucket[f]++;
 		}
		for (int i = 0; i < NBUCKETS; ++i) {
			printf("[%d] %d  ", i, bucket[i]);
		}
		printf("\n");
		for (int i = 0; i < NBUCKETS; ++i) {
			printf("[%f] %d  ", float(i)/4.0f, fbucket[i]);
		}
		printf("\n");
	}

	return 0;
}
