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


float PerlinNoise::Noise(float _x, float _y, float _z) 
{
	unsigned X = unsigned( floorf(_x) ); 
	unsigned Y = unsigned( floorf(_y) ); 
	unsigned Z = unsigned( floorf(_z) );

	float x = _x - (float)X;
	float y = _y - (float)Y;
	float z = _z - (float)Z;

	X &= 255;
	Y &= 255;
	Z &= 255;

	float u = Fade5( x );
	float v = Fade5( y );
	float w = Fade5( z );

	unsigned A  = p[X  ]+Y;
	unsigned AA = p[A]+Z;
	unsigned AB = p[A+1]+Z;
	unsigned B  = p[X+1]+Y;
	unsigned BA = p[B]+Z;
	unsigned BB = p[B+1]+Z;

	float n=_Lerp(w, _Lerp(v, _Lerp(u, Grad(p[AA  ], x  , y  , z   ),  
									   Grad(p[BA  ], x-1, y  , z   )), 
					          _Lerp(u, Grad(p[AB  ], x  , y-1, z   ),  
									   Grad(p[BB  ], x-1, y-1, z   ))),
					 _Lerp(v, _Lerp(u, Grad(p[AA+1], x  , y  , z-1 ),  
									   Grad(p[BA+1], x-1, y  , z-1 )), 
							  _Lerp(u, Grad(p[AB+1], x  , y-1, z-1 ),
									   Grad(p[BB+1], x-1, y-1, z-1 ))));
	GLASSERT( n >= -1 && n <= 1 );
	return n;
}
