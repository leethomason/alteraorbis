#include <stdio.h>
#include <stdlib.h>

#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcolor.h"
#include "../shared/lodepng.h"

using namespace grinliz;

static const int DOMAIN_SIZE = 256;
static const int NUM_TESTS = 5;
static const int MAX_HEIGHT = 4;
static const int NUM_GENS = 4;

enum {
	USE_MAX_HEIGHT,		// shear walls to max height
	NOISE_HEIGHT,		// height from separate generator
	USE_HIGH_HEIGHT,	// noise from separate generator, but high
	KEEP_HEIGHT,		// height from noise generator

	PLASMA_HEIGHT,
	FRACTAL_HEIGHT
};


float Fractal( PerlinNoise* noise, int x, int y, int octave0, int octave1, float octave1Amount )
{
	static const float DS_INV = 1.0f / (float)DOMAIN_SIZE;
	const float OCTAVE_0 = (float)octave0;
	const float OCTAVE_1 = (float)octave1;

	float nx0 = OCTAVE_0 * (float)x * DS_INV;
	float ny0 = OCTAVE_0 * (float)y * DS_INV;

	float nx1 = OCTAVE_1 * (float)x * DS_INV;
	float ny1 = OCTAVE_1 * (float)y * DS_INV;

	float n = 1.0f - fabs( noise[0].Noise( nx0, ny0, 0 ) - noise[1].Noise( nx0, ny0, 0 ) );
	n = Clamp( n, -1.0f, 1.0f );
	return n;
}


void ApplyThreshold( Color4U8* pixels, float targetFractionBlack, PerlinNoise* height, int heightMap )
{
	float high = 1.0f;
	float low  = 0.0f;
	float fractionBlack = 0.0f;
	int midC=0;

	for( int it=0; it<6; ++it ) {
		float mid = Mean( high, low );
		midC = (int)(255.f * mid);
		int count = 0;
		for( int i=0; i<DOMAIN_SIZE*DOMAIN_SIZE; ++i ) {
			if ( pixels[i].r < midC )
				++count;
		}
		fractionBlack = (float)count/(float)(DOMAIN_SIZE*DOMAIN_SIZE);

		if ( targetFractionBlack > fractionBlack ) {
			low = mid;
		}
		else {
			high = mid;
		}
	}
	for( int y=0; y<DOMAIN_SIZE; ++y ) {
		for( int x=0; x<DOMAIN_SIZE; ++x ) {
			int i = y*DOMAIN_SIZE+x;
			if ( pixels[i].r < midC ) {
				pixels[i].Set( 0, 0, 0, 255 );
			}
			else {
				if ( heightMap == USE_MAX_HEIGHT ) {
					pixels[i].Set( 255, 255, 255, 255 );
				}
				else if ( heightMap == KEEP_HEIGHT ) {
					int h = 255 * (pixels[i].r - midC) / (255 - midC);
					//int h = pixels[i].r;
					pixels[i].Set( h, h, h, 255 );
				}
				else if ( heightMap == NOISE_HEIGHT || heightMap == USE_HIGH_HEIGHT ) {

					float hf = height->Noise( (float)x, (float)y, 0.0f, (float)DOMAIN_SIZE, 8, 32, 0.3f ); 
					hf = Clamp( hf*0.5f + 0.5f, 0.0f, 1.0f );

					int   h=0;
					if ( heightMap == USE_HIGH_HEIGHT )
						h = int( 128.0f + 127.0f*hf );
					else
						h  = int( hf*255.0f );

					pixels[i].Set( h, h, h, 255 );
				};
			}
		}
	}
	printf( "midC=%d target=%.2f actual=%.2f\n", midC, targetFractionBlack, fractionBlack );
}


int main(int argc, const char* argv[])
{
	bool applyThreshold			= true;
	const float FRACTION_CLEAR	= 0.55f;
	const int style				= FRACTAL_HEIGHT;
//	int heightMap				= USE_MAX_HEIGHT; 
//	int heightMap				= USE_HIGH_HEIGHT;
//	int heightMap				= KEEP_HEIGHT;
	int heightMap				= NOISE_HEIGHT;

	Color4U8* pixels = new Color4U8[ DOMAIN_SIZE*DOMAIN_SIZE ];

	for( int test=0; test<NUM_TESTS; ++test ) {

		PerlinNoise arr[NUM_GENS] = { 
			PerlinNoise( 42000 + 192837*test ), 
			PerlinNoise( 37 + 14235*test ),
			PerlinNoise( 38103 + 3910*test ),
			PerlinNoise( test )
		};
		PerlinNoise heightNoise( 39393 + test*112 );

		for( int y=0; y<DOMAIN_SIZE; ++y ) {
			for( int x=0; x<DOMAIN_SIZE; ++x ) {

				float n = 0;
				if ( style == PLASMA_HEIGHT ) {
					n = arr[0].Noise( (float)x, (float)y, 0.0f, (float)DOMAIN_SIZE, 8, 32, 0.3f );	// Plasma rocks.
				}
				else if ( style == FRACTAL_HEIGHT ) {
					// Canyon.
					n = -Fractal( arr, x, y, 8, 16, 0.2f );			// Good tunnel-canyon look
					//n +=       Simple( arr+2, x, y, 16, 32, 0.5 )*0.1f;	// smooth variation
					n +=       arr[2].Noise( (float)x, (float)y, 0, (float)DOMAIN_SIZE, 32, 64, 0.5f )*0.1f;		// rough variation
					n = Clamp( n, -1.0f, 1.0f );
				}
				int v = (int)(255.0f * (0.5f*n+0.5f));
				Color4U8 c = { v, v, v, 255 };
				pixels[y*DOMAIN_SIZE+x] = c;
			}
		}

		if ( applyThreshold ) {
			ApplyThreshold( pixels, FRACTION_CLEAR, &heightNoise, heightMap );
		}

		CStr<32> str;
		str.Format( "rock%d.png", test );
		lodepng_encode32_file( str.c_str(), (const unsigned char*)pixels, DOMAIN_SIZE, DOMAIN_SIZE );
	}
	delete [] pixels;
	return 0;
}
