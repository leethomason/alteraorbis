#include <stdio.h>
#include <stdlib.h>

#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcolor.h"
#include "../shared/lodepng.h"

#include "../script/rockgen.h"
#include "../game/worldgrid.h"

using namespace grinliz;

static const int DOMAIN_SIZE = 1024;

int main(int argc, const char* argv[])
{
	static const int NUM_TESTS  = 1;
	bool applyThreshold			= true;
	const float FRACTION_CLEAR	= 0.55f;
	const int height			= RockGen::NOISE_HEIGHT;

	Color4U8* pixels = new Color4U8[ DOMAIN_SIZE*DOMAIN_SIZE ];

	for( int test=0; test<NUM_TESTS; ++test ) {
		printf( "Test %d starting.\n", test );
		RockGen rockGen( DOMAIN_SIZE );

		printf( "Calc" );
		rockGen.StartCalc( 0 );
		for( int y=0; y<DOMAIN_SIZE; ++y ) {
			rockGen.DoCalc( y );
			if ( y%32 == 0 ) {
				printf( "." ); fflush(stdout);
			}
		}
		rockGen.EndCalc();
		printf( "Done. Threshold...\n" );

		if ( applyThreshold ) {
			rockGen.DoThreshold( 14+33*test, FRACTION_CLEAR, height );
		}
		printf( "Done.\n" );

		const U8* heightMap = rockGen.Height();

		for( int y=0; y<DOMAIN_SIZE; ++y ) {
			for( int x=0; x<DOMAIN_SIZE; ++x ) {

				WorldGrid grid;
				memset( &grid, 0, sizeof(WorldGrid) );

				int v = heightMap[y*DOMAIN_SIZE+x]/57;
				GLASSERT( v >= 0 && v <= 4 );
				Color4U8 c = { 0, 180, 0, 255 };
				if ( v ) {
					c.Set( 100+v*35, 100+v*35, 100+v*35, 255 );
				}
				pixels[y*DOMAIN_SIZE+x] = c;
			}
		}

		printf( "Test %d saving.\n", test );
		CStr<32> str;
		str.Format( "rock%d.png", test );
		lodepng_encode32_file( str.c_str(), (const unsigned char*)pixels, DOMAIN_SIZE, DOMAIN_SIZE );
	}
	delete [] pixels;
	return 0;
}
