#include "rockgen.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glrandom.h"

#include <stdio.h>

using namespace grinliz;

RockGen::RockGen( int sz ) : size(sz)
{
	heightMap = new U8[size*size];
}


RockGen::~RockGen()
{
	delete [] heightMap;
}


float RockGen::Fractal( int x, int y, float octave0, float octave1, float octave1Amount )
{
	const float INV = 1.0f / (float)size;
	const float OCTAVE_0 = octave0;
	const float OCTAVE_1 = octave1;

	float nx0 = OCTAVE_0 * (float)x * INV;
	float ny0 = OCTAVE_0 * (float)y * INV;

	float nx1 = OCTAVE_1 * (float)x * INV;
	float ny1 = OCTAVE_1 * (float)y * INV;

	float n = 1.0f - fabs( noise0->Noise( nx0, ny0, 0 ) - noise1->Noise( nx1, ny1, 0 ) );
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
	int maxC = 0;

	for( int it=0; it<6; ++it ) {
		float mid = Mean( high, low );
		midC = (int)(255.f * mid);
		int count = 0;
		for( int i=0; i<size*size; ++i ) {
			int h = heightMap[i];
			maxC = Max( maxC, h );
			if ( h < midC )
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
				if ( heightStyle == KEEP_HEIGHT ) {
					int h = 255 * (heightMap[i] - midC) / (maxC - midC);
					heightMap[i] = Max( h, 1 );
				}
				else if ( heightStyle == NOISE_HEIGHT || heightStyle == NOISE_HIGH_HEIGHT ) {

					const float OCTAVE8  = (float)size / 32;
					//const float OCTAVE16 = (float)size / 16;
					//const float OCTAVE32 = (float)size / 8;	
					const float OCTAVE64 = (float)size / 3;	
					const float SECOND = 0.6f;

					float hf = noise.Noise2( (float)x, (float)y, (float)size, OCTAVE8, OCTAVE64, SECOND ); 
					// Convert to [0,1]
					hf = Clamp( hf*0.5f + 0.5f, 0.0f, 1.0f );

					int   h=0;
					if ( heightStyle == NOISE_HIGH_HEIGHT )
						h = int( 64.0f + 191.0f*hf );
					else
						h  = int( hf*255.0f );

					heightMap[i] = Max( h, 1 );
				}
			}
		}
	}

	printf( "midC=%d target=%.2f actual=%.2f\n", midC, targetFractionBlack, fractionBlack );
}


void RockGen::StartCalc( int seed )
{
	Random random( seed );
	noise0 = new PerlinNoise( random.Rand() );
	noise1 = new PerlinNoise( random.Rand() );
}


void RockGen::EndCalc()
{
	delete noise0;
	delete noise1;
}


void RockGen::DoCalc( int y )
{
	const float OCTAVE4  = (float)size / 64;
	const float OCTAVE8  = (float)size / 32;
	const float OCTAVE16 = (float)size / 16;
	const float OCTAVE32 = (float)size / 8;	
	const float OCTAVE64 = (float)size / 4;	
	const float SECOND = 0.3f;
	static const int rockStyle = CAVEY;

	float t = (float)y/(float)(size-1);
	float octave	 = Lerp( OCTAVE4, OCTAVE8, t );
	float octaveLow  = Lerp( OCTAVE16, OCTAVE32, t );
	float octaveHigh = Lerp( OCTAVE32, OCTAVE64, t );

	for( int x=0; x<size; ++x ) {

		float n = 0;
		if ( rockStyle == BOULDERY ) {
			n = noise0->Noise2( (float)x, (float)y, (float)size, OCTAVE8, OCTAVE32, 0.5f );	// Plasma rocks.
		}
		else if ( rockStyle == CAVEY ) {
			// Canyon.
			n = -Fractal( x, y, octave, OCTAVE16, 0.2f );		// Good tunnel-canyon look
			n += noise1->Noise2( (float)x, (float)y, (float)size, octaveLow, octaveHigh, 0.5 )*SECOND;
			
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

