#ifndef GRINLIZ_NOISE_INCLUDED
#define GRINLIZ_NOISE_INCLUDED

#include "glutil.h"

namespace grinliz {

class PerlinNoise
{
public:
	PerlinNoise( U32 seed );
	
	// [-1,1]
	float Noise(float _x, float _y, float _z) const;
	float Noise2(float _x, float _y ) const;

	float Noise( float x, float y, float z, float size, int octave0, int octave1, float octave1Amount ) const
	{
		const float INV      = 1.0f / size;
		const float OCTAVE_0 = (float)octave0;
		const float OCTAVE_1 = (float)octave1;

		float nx0 = OCTAVE_0 * (float)x * INV;
		float ny0 = OCTAVE_0 * (float)y * INV;
		float nz0 = OCTAVE_0 * (float)z * INV;

		float nx1 = OCTAVE_1 * (float)x * INV;
		float ny1 = OCTAVE_1 * (float)y * INV;
		float nz1 = OCTAVE_1 * (float)z * INV;

		float n = Noise( nx0, ny0, nz0 ) + Noise( nx1, ny1, nz1 ) * octave1Amount;
		return Clamp( n, -1.0f, 1.0f );
	}

	/*
	float NormNoise( float x, float y, float z ) const {
		return Normalize( Noise( x, y, z ) );
	}

	float AbsNoise( float x, float y, float z ) const {
		float v = Noise( x, y, z );
		return -1.0f + 2.0f*fabsf( v );
	}
	*/

	// Convert [-1,1] -> [0,1]
	static float Normalize( float n ) { return n*0.5f + 0.5f; }

private:
	unsigned p[512];

	inline static float Grad( unsigned hash, float x, float y, float z) {
		unsigned h = hash & 15;                      
		float u = h<8 ? x : y;
		float v = h<4 ? y : (h==12||h==14) ? x : z;
		float result = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
		return result;
	}
	inline static float Grad2( unsigned hash, float x, float y) {
		unsigned h = hash & 15;                      
		float u = h<8 ? x : y;
		float v = h<4 ? y : (h==12||h==14) ? x : 0;
		float result = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
		return result;
	}
	// Grr. Non-standard args.
	inline static float _Lerp( float t, float a, float b ) { 
		return Lerp( a, b, t ); 
	}

};


};

#endif // GRINLIZ_NOISE_INCLUDED

