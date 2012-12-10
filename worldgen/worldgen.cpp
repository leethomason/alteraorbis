// worldgen.cpp : Defines the entry point for the console application.
//

#include <ctime>

#include "../shared/lodepng.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

static const int WIDTH  = 1024;
static const int HEIGHT = 1024;

static const int COUNT = 5;

static const float BASE0 = 8.f;
static const float BASE1 = 64.f;
static const float EDGE = 0.1f;
static const float OCTAVE = 0.18f;

static const float FRACTION_LAND = 0.4f;

int CountFlixelsAboveCutoff( const float* flixels, float cutoff )
{
	int count = 0;
	for( int j=0; j<HEIGHT; ++j ) {
		for( int i=0; i<WIDTH; ++i ) {
			if ( flixels[j*WIDTH+i] > cutoff )
				++count;
		}
	}
	return count;
}

// 37840

int main(int argc, const char* argv[])
{

	//Random random;
	//random.SetSeedFromTime();
	int seed = 0; //random.Rand();

	if ( argc >= 2 ) {
		seed = atoi( argv[1] );
	}

	Color4U8* pixels = new Color4U8[WIDTH*HEIGHT];
	memset( pixels, 0xa0, sizeof(Color4U8)*WIDTH*HEIGHT );
	float* flixels = new float[WIDTH*HEIGHT];

	clock_t startTime = clock();
	clock_t loopTime = startTime;

	for( int i=0; i<COUNT; ++i ) {
		PerlinNoise noise0( i*12345 );
		PerlinNoise noise1( i*12345 );

		clock_t startPerlin = clock();
		for( int j=0; j<HEIGHT; ++j ) {
			for( int i=0; i<WIDTH; ++i ) {

				float nx = (float)i/(float)WIDTH;
				float ny = (float)j/(float)HEIGHT;

				// Noise layer.
				float n0 = noise0.Noise( BASE0*nx, BASE0*ny, nx );
				float n1 = noise1.Noise( BASE1*nx, BASE1*ny, nx );

				float d2 = (nx-0.5f)*(nx-0.5f) + (ny-0.5f)*(ny-0.5f);
				//float octave = Lerp( 0.0f, 0.3f, d2 );
				float octave = OCTAVE;

				//float n = Clamp( n0 + n1*octave, -1.0f, 1.0f );
				float n = n0 + n1*octave;
				n = PerlinNoise::Normalize( n );

				// Water at the edges.
				float dEdge = Min( nx, 1.0f-nx );
				if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );
				dEdge = Min( ny, 1.0f-ny );
				if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );

				flixels[j*WIDTH+i] = n;
			}
		}
		clock_t endPerlin = clock();

		float cutoff = FRACTION_LAND;
		int target = (int)((float)(WIDTH*HEIGHT)*FRACTION_LAND);
		float high = 1.0f;
		int highCount = 0;
		float low = 0.0f;
		int lowCount = WIDTH*HEIGHT;
		int iteration=0;

		while( true ) {
			int count = CountFlixelsAboveCutoff( flixels, cutoff );
			if ( count > target ) {
				low = cutoff;
				lowCount = count;
			}
			else {
				high = cutoff;
				highCount = count;
			}
			if ( abs(count - target) < (target/10) ) {
				break;
			}
			//cutoff = (high+low)*0.5f;
			cutoff = low + (high-low)*(float)(lowCount - target) / (float)(lowCount - highCount);
			++iteration;
		}

		for( int j=0; j<HEIGHT; ++j ) {
			for( int i=0; i<WIDTH; ++i ) {
				int gray = flixels[j*WIDTH+i] > cutoff ? 255 : 0;
				pixels[j*WIDTH+i].Set( gray, gray, gray, 255 );
			}
		}
		clock_t endTime = clock();
		printf( "loop %d: perlin=%dms iteration=%d %dms\n", i, 
				endPerlin - startPerlin, 
				iteration, 
				endTime - loopTime );
		loopTime = endTime;

		CStr<32> fname;
		fname.Format( "worldgen%02d.png", i );
		lodepng_encode32_file( fname.c_str(), (const unsigned char*)pixels, WIDTH, HEIGHT );
	}
	printf( "total time %dms\n", clock()-startTime );
	delete [] pixels;
	delete [] flixels;
	return 0;
}

