#ifndef GRINLIZ_NOISE_INCLUDED
#define GRINLIZ_NOISE_INCLUDED

#include "glutil.h"

namespace grinliz {

class PerlinNoise
{
public:
	// If halfRange, then noise: [0,1], else noise [-1,-1]
	PerlinNoise( U32 seed );
	
	float Noise(float _x, float _y, float _z);
	static float Normalize( float n ) { return n*0.5f + 0.5f; }

private:
	unsigned p[512];

	inline static float Grad( unsigned hash, float x, float y, float z) {
		unsigned h = hash & 15;                      
		float u = h<8 ? x : y;
		float v = h<4 ? y : (h==12||h==14) ? x : z;
		return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
	}
	// Grr. Non-standard args.
	inline static float _Lerp( float t, float a, float b ) { return Lerp( a, b, t ); }

};


};

#endif // GRINLIZ_NOISE_INCLUDED

