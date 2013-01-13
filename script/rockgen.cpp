#include "rockgen.h"

using namespace grinliz;

RockGen::RockGen( int sz ) : size(sz)
{
	heightMap = new U8[size*size];
}


RockGen::~RockGen()
{
	delete [] heightMap;
}


float RockGen::Fractal( const PerlinNoise* noise, int x, int y, int octave0, int octave1, float octave1Amount )
{
	const float INV = 1.0f / (float)size;
	const float OCTAVE_0 = (float)octave0;
	const float OCTAVE_1 = (float)octave1;

	float nx0 = OCTAVE_0 * (float)x * INV;
	float ny0 = OCTAVE_0 * (float)y * INV;

	float nx1 = OCTAVE_1 * (float)x * INV;
	float ny1 = OCTAVE_1 * (float)y * INV;

	float n = 1.0f - fabs( noise[0].Noise( nx0, ny0, 0 ) - noise[1].Noise( nx0, ny0, 0 ) );
	n = Clamp( n, -1.0f, 1.0f );
	return n;
}


void RockGen::DoThreshold( int seed, float targetFractionBlack, int heightStyle )
{
	PerlinNoise noise( 39393 + seed*112 );
	float high = 1.0f;
	float low  = 0.0f;
	float fractionBlack = 0.0f;
	int midC=0;

	for( int it=0; it<6; ++it ) {
		float mid = Mean( high, low );
		midC = (int)(255.f * mid);
		int count = 0;
		for( int i=0; i<size*size; ++i ) {
			if ( heightMap[i] < midC )
				++count;
		}
		fractionBlack = (float)count/(float)(size*size);

		if ( targetFractionBlack > fractionBlack ) {
			low = mid;
		}
		else {
			high = mid;
		}
	}
	for( int y=0; y<size; ++y ) {
		for( int x=0; x<size; ++x ) {
			int i = y*size+x;
			if ( heightMap[i] < midC ) {
				heightMap[i] = 0;
			}
			else {
				if ( heightStyle == USE_MAX_HEIGHT ) {
					heightMap[i] = 255;
				}
				else if ( heightStyle == KEEP_HEIGHT ) {
					int h = 255 * (heightMap[i] - midC) / (255 - midC);
					heightMap[i] = h;
				}
				else if ( heightStyle == NOISE_HEIGHT || heightStyle == USE_HIGH_HEIGHT ) {

					float hf = noise.Noise( (float)x, (float)y, 0.0f, (float)size, 8, 32, 0.3f ); 
					hf = Clamp( hf*0.5f + 0.5f, 0.0f, 1.0f );

					int   h=0;
					if ( heightStyle == USE_HIGH_HEIGHT )
						h = int( 128.0f + 127.0f*hf );
					else
						h  = int( hf*255.0f );

					heightMap[i] = h;
				};
			}
		}
	}
	//printf( "midC=%d target=%.2f actual=%.2f\n", midC, targetFractionBlack, fractionBlack );
}



void RockGen::DoCalc( int seed, int rockStyle )
{
	PerlinNoise arr[2] = { 
		PerlinNoise( 42000 + 13*seed ), 
		PerlinNoise( 37 + 4*seed ),
	};

	const int OCTAVE8  = size / 32;
	const int OCTAVE16 = size / 16;
	const int OCTAVE32 = size / 8;	
	const int OCTAVE64 = size / 4;	

	for( int y=0; y<size; ++y ) {
		for( int x=0; x<size; ++x ) {

			float n = 0;
			if ( rockStyle == BOULDERY ) {
				n = arr[0].Noise( (float)x, (float)y, 0.0f, (float)size, OCTAVE8, OCTAVE32, 0.3f );	// Plasma rocks.
			}
			else if ( rockStyle == CAVEY_SMOOTH || rockStyle == CAVEY_ROUGH ) {
				// Canyon.
				n = -Fractal( arr, x, y, OCTAVE8, OCTAVE16, 0.2f );		// Good tunnel-canyon look
				if ( rockStyle == CAVEY_SMOOTH )
					n +=       arr[1].Noise( (float)x, (float)y, 0.0f, (float)size, OCTAVE16, OCTAVE32, 0.5 )*0.1f;
				else
					n +=       arr[1].Noise( (float)x, (float)y, 0.5f, (float)size, OCTAVE32, OCTAVE64, 0.5f )*0.1f;
			
				n = Clamp( n, -1.0f, 1.0f );
			}
			else {
				GLASSERT( 0 );
			}
			int v = (int)(255.0f * (0.5f*n+0.5f));
			GLASSERT( v >= 0 && v <= 255 );
			heightMap[y*size+x] = v;
		}
	}
}

