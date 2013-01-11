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
static const int HEIGHT_TO_COLOR = 60;
static const int NUM_GENS = 4;


int ToHeight( float n, float t ) {
	if ( n >= t ) {
		float hf = (float)(MAX_HEIGHT) * (n - t) / ( 1.01f - t ); // always evals < 1
		int h = (int)hf;
		GLASSERT( h >= 0 && h < MAX_HEIGHT );
		return h+1;
	}
	return 0;
}


float Simple( PerlinNoise* noise, int x, int y, int octave0, int octave1, float octave1Amount )
{
	static const float THRESH = 0.1f;

	static const float DS_INV = 1.0f / (float)DOMAIN_SIZE;
	const float OCTAVE_0 = (float)octave0;
	const float OCTAVE_1 = (float)octave1;

	float nx0 = OCTAVE_0 * (float)x * DS_INV;
	float ny0 = OCTAVE_0 * (float)y * DS_INV;

	float nx1 = OCTAVE_1 * (float)x * DS_INV;
	float ny1 = OCTAVE_1 * (float)y * DS_INV;

	float n = noise->Noise( nx0, ny0, 0 ) + noise->Noise( nx1, ny1, 0.5f ) * octave1Amount;
	n = Clamp( n, -1.0f, 1.0f );
	return n;
}


float Fractal( PerlinNoise* noise, int x, int y, int octave0, int octave1, float octave1Amount )
{
	static const float THRESH = 0.1f;

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


int main(int argc, const char* argv[])
{

	Color4U8* pixels = new Color4U8[ DOMAIN_SIZE*DOMAIN_SIZE ];

	for( int test=0; test<NUM_TESTS; ++test ) {

		PerlinNoise arr[NUM_GENS] = { 
			PerlinNoise( 42000 + 192837*test ), 
			PerlinNoise( 37 + 14235*test ),
			PerlinNoise( 38103 + 3910*test ),
			PerlinNoise( test )
		};

		bool convertToHeight   = true;
		bool heightIndependent = true;
		static const float THRESHOLD = -0.7f;

		for( int y=0; y<DOMAIN_SIZE; ++y ) {
			for( int x=0; x<DOMAIN_SIZE; ++x ) {

				//int h = Simple( &noise, x, y, 8, 32, 0.3f );
				//int h = Fractal( &noise, x, y, 8, 32, 0.3f );
				//Color4U8 c = {  h*HEIGHT_TO_COLOR, h*HEIGHT_TO_COLOR, h*HEIGHT_TO_COLOR, 255 };

				float n = -Fractal( arr, x, y, 8, 16, 0.5f ) + Simple( arr, x, y, 32, 0, 0 )*0.3f;
				int v = (int)(255.0f * (0.5f*n+0.5f));
				Color4U8 c = { v, v, v, 255 };

				if ( convertToHeight ) {
					int h = 0;
					if ( heightIndependent ) {
						h = n > THRESHOLD ? MAX_HEIGHT : 0;
					}
					else {
						h = ToHeight( n, THRESHOLD );
					}
					c.Set( h*HEIGHT_TO_COLOR, h*HEIGHT_TO_COLOR, h*HEIGHT_TO_COLOR, 255 );
				}
				pixels[y*DOMAIN_SIZE+x] = c;
			}
		}
		CStr<32> str;
		str.Format( "rock%d.png", test );
		lodepng_encode32_file( str.c_str(), (const unsigned char*)pixels, DOMAIN_SIZE, DOMAIN_SIZE );
	}
	delete [] pixels;
	return 0;
}
