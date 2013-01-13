#include "glnoise.h"
#include "glrandom.h"


using namespace grinliz;

PerlinNoise::PerlinNoise( U32 seed )
{
	Random random( seed );
	for( int i=0; i<256; ++i ) {
		p[i] = i;
	}
	for( int i=0; i<256; ++i ) {
		Swap( &p[i], &p[random.Rand(256)] );
	}
	for( int i=256; i<512; ++i ) {
		p[i] = p[i-256];
	}
}


float PerlinNoise::Noise(float x, float y, float z) const
{
	int X = int( floorf(x) ) & 255; 
	int Y = int( floorf(y) ) & 255; 
	int Z = int( floorf(z) ) & 255;

	x -= floorf( x );
	y -= floorf( y );
	z -= floorf( z );

	float u = Fade5( x );
	float v = Fade5( y );
	float w = Fade5( z );

	int A  = p[X  ]+Y;
	int AA = p[A  ]+Z;
	int AB = p[A+1]+Z;
	int B  = p[X+1]+Y;
	int BA = p[B  ]+Z;
	int BB = p[B+1]+Z;

	float n=_Lerp(w, _Lerp(v, _Lerp(u, Grad(p[AA  ], x  , y  , z   ),  
									   Grad(p[BA  ], x-1, y  , z   )), 
					          _Lerp(u, Grad(p[AB  ], x  , y-1, z   ),  
									   Grad(p[BB  ], x-1, y-1, z   ))),
					 _Lerp(v, _Lerp(u, Grad(p[AA+1], x  , y  , z-1 ),  
									   Grad(p[BA+1], x-1, y  , z-1 )), 
							  _Lerp(u, Grad(p[AB+1], x  , y-1, z-1 ),
									   Grad(p[BB+1], x-1, y-1, z-1 ))));
	// Mysteries of floating point: occasionally, slightly overflows.
	//GLASSERT( n >= -1.01f && n <= 1.01f );
	return n;
}
