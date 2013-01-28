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

static const int DOMAIN_SIZE = 256;
static const int NUM_TESTS = 5;

int main(int argc, const char* argv[])
{
	bool applyThreshold			= true;
	const float FRACTION_CLEAR	= 0.55f;
	const int style				= RockGen::BOULDERY;
	const int height			= RockGen::NOISE_HIGH_HEIGHT;

	Color4U8* pixels = new Color4U8[ DOMAIN_SIZE*DOMAIN_SIZE ];

	for( int test=0; test<NUM_TESTS; ++test ) {
		printf( "Test %d starting.\n", test );
		RockGen rockGen( DOMAIN_SIZE );
		rockGen.DoCalc( 12+7*test, style );

		if ( applyThreshold ) {
			rockGen.DoThreshold( 14+33*test, FRACTION_CLEAR, height );
		}

		const U8* heightMap = rockGen.Height();

		for( int y=0; y<DOMAIN_SIZE; ++y ) {
			for( int x=0; x<DOMAIN_SIZE; ++x ) {

				WorldGrid grid;
				memset( &grid, 0, sizeof(WorldGrid) );

				int v = heightMap[y*DOMAIN_SIZE+x];
				if ( v == 0 )
					v = 1;
				grid.SetLandAndRock( v );
				pixels[y*DOMAIN_SIZE+x] = grid.ToColor();
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