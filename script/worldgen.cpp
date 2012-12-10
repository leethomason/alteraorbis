#include "worldgen.h"
#include <string.h>
#include "../grinliz/glnoise.h"

using namespace grinliz;

static const float BASE0 = 8.f;
static const float BASE1 = 64.f;
static const float EDGE = 0.1f;
static const float OCTAVE = 0.18f;

WorldGen::WorldGen()
{
	land = new U8[SIZE*SIZE];
	memset( land, 0, SIZE*SIZE );
	color = new U8[SIZE*SIZE];
	memset( color, 0, SIZE*SIZE );
}


WorldGen::~WorldGen()
{
	delete [] land;
	delete [] color;
}


bool WorldGen::CalcLandAndWater( U32 seed0, U32 seed1, float fractionLand )
{
	float* flixels = new float[SIZE*SIZE];

	PerlinNoise noise0( seed0 );
	PerlinNoise noise1( seed1 );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {

			float nx = (float)i/(float)SIZE;
			float ny = (float)j/(float)SIZE;

			// Noise layer.
			float n0 = noise0.Noise( BASE0*nx, BASE0*ny, nx );
			float n1 = noise1.Noise( BASE1*nx, BASE1*ny, nx );

			float n = n0 + n1*OCTAVE;
			n = PerlinNoise::Normalize( n );

			// Water at the edges.
			float dEdge = Min( nx, 1.0f-nx );
			if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );
			dEdge = Min( ny, 1.0f-ny );
			if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );

			flixels[j*SIZE+i] = n;
		}
	}

	float cutoff = fractionLand;
	int target = (int)((float)(SIZE*SIZE)*fractionLand);
	float high = 1.0f;
	int highCount = 0;
	float low = 0.0f;
	int lowCount = SIZE*SIZE;
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

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			land[j*SIZE+i] = flixels[j*SIZE+i] > cutoff ? 1 : 0;
		}
	}
	delete [] flixels;
	return true;
}


int WorldGen::CountFlixelsAboveCutoff( const float* flixels, float cutoff )
{
	int count = 0;
	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			if ( flixels[j*SIZE+i] > cutoff )
				++count;
		}
	}
	return count;
}



