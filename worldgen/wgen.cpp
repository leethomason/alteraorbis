// worldgen.cpp : Defines the entry point for the console application.
//

#include <ctime>

#include "../shared/lodepng.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../script/worldgen.h"

using namespace grinliz;

static const int COUNT = 5;
static const float FRACTION_LAND = 0.4f;
static const int WIDTH  = WorldGen::SIZE;
static const int HEIGHT = WorldGen::SIZE;


int main(int argc, const char* argv[])
{

	int seed = 0; //random.Rand();
	int count = COUNT;

	if ( argc >= 2 ) {
		seed = atoi( argv[1] );
		count = 1;
	}

	Color4U8* pixels = new Color4U8[WIDTH*HEIGHT];
	memset( pixels, 0xa0, sizeof(Color4U8)*WIDTH*HEIGHT );

	clock_t startTime = clock();
	clock_t loopTime = startTime;

	WorldGen worldGen;
	U32 seed0 = seed;
	U32 seed1 = seed*43+1924;

	for( int i=0; i<count; ++i ) {
		worldGen.CalcLandAndWater( seed0, seed1, FRACTION_LAND );
		const U8* land = worldGen.Land();
	
		for( int j=0; j<HEIGHT; ++j ) {
			for( int i=0; i<WIDTH; ++i ) {
				int gray = land[j*WIDTH+i] ? 255 : 0;
				pixels[j*WIDTH+i].Set( gray, gray, gray, 255 );
			}
		}
		clock_t endTime = clock();
		printf( "loop %d: %dms\n", i, endTime - loopTime );
		loopTime = endTime;

		CStr<32> fname;
		fname.Format( "worldgen%02d.png", i );
		lodepng_encode32_file( fname.c_str(), (const unsigned char*)pixels, WIDTH, HEIGHT );

		seed0 = seed0*3+7;
		seed1 = seed1*11+2;
	}
	printf( "total time %dms\n", clock()-startTime );
	return 0;
}

