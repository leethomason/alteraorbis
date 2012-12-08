// worldgen.cpp : Defines the entry point for the console application.
//

#include "../shared/lodepng.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"

using namespace grinliz;

static const int WIDTH = 512;
static const int HEIGHT = 512;

static const float BASE0 = 4.f;
static const float BASE1 = 64.f;
static const float EDGE = 0.1f;

static const float FRACTION_LAND = 0.5f;

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


int main(int argc, const char* argv[])
{

	Random random;
	random.SetSeedFromTime();
	int seed = random.Rand();

	if ( argc >= 2 ) {
		seed = atoi( argv[1] );
	}

	Color4U8* pixels = new Color4U8[WIDTH*HEIGHT];
	memset( pixels, 0xa0, sizeof(Color4U8)*WIDTH*HEIGHT );
	float* flixels = new float[WIDTH*HEIGHT];

	PerlinNoise noise( seed );
	for( int j=0; j<HEIGHT; ++j ) {
		for( int i=0; i<WIDTH; ++i ) {

			float nx = (float)i/(float)WIDTH;
			float ny = (float)j/(float)HEIGHT;

			// Noise layer.
			float n0 = noise.Noise( BASE0*nx, BASE0*ny, 0.0f );
			float n1 = noise.Noise( BASE1*nx, BASE1*ny, 1.0f );

			float d2 = (nx-0.5f)*(nx-0.5f) + (ny-0.5f)*(ny-0.5f);
			float octave = Lerp( 0.0f, 0.3f, d2 );

			float n = Clamp( n0 + n1*octave, -1.0f, 1.0f );
			n = PerlinNoise::Normalize( n );

			// Water at the edges.
			float dEdge = Min( nx, 1.0f-nx );
			if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );
			dEdge = Min( ny, 1.0f-ny );
			if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );

			flixels[j*WIDTH+i] = n;
		}
	}

	float cutoff = FRACTION_LAND;
	int target = (int)((float)(WIDTH*HEIGHT)*FRACTION_LAND);

	/*
	for( int i=0; i<3; ++i ) {
		int count = CountFlixelsAboveCutoff( flixels, cutoff );

		int rise = WIDTH*HEIGHT - count;
		float run = 1.0f - cutoff;
		float slope = (float)rise / run;

		cutoff += slope * (float)(target-count)/(float)(WIDTH*HEIGHT);
		cutoff = Clamp( cutoff, 0.001f, 0.99f );
	}
	*/

	for( int j=0; j<HEIGHT; ++j ) {
		for( int i=0; i<WIDTH; ++i ) {
			int gray = flixels[j*WIDTH+i] > cutoff ? 255 : 0;
			pixels[j*WIDTH+i].Set( gray, gray, gray, 255 );
		}
	}

	lodepng_encode32_file( "worldgen.png", (const unsigned char*)pixels, WIDTH, HEIGHT );
	delete [] pixels;
	delete [] flixels;
	return 0;
}

