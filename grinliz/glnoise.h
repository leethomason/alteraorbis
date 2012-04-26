#ifndef GRINLIZ_NOISE_INCLUDED
#define GRINLIZ_NOISE_INCLUDED

#include "glutil.h"

namespace grinliz {

class PerlinNoise
{
public:
	PerlinNoise( U32 seed );

	float noise(float _x, float _y, float _z) {
		int X = LRintf( floorf(_x) ) & 255; 
		int	Y = LRintf( floorf(_y) ) & 255, 
		int	Z = LRintf( floorf(_z) ) & 255;

		float x -= (float)X;
		float y -= (float)Y;
		float z -= (float)Z;

		float u = Fade5( x );
		float v = Fade5( y );
		float w = Fade5( z );

		int A = p[X  ]+Y, AA = p[A]+Z, AB = p[A+1]+Z,      // HASH COORDINATES OF
          B = p[X+1]+Y, BA = p[B]+Z, BB = p[B+1]+Z;      // THE 8 CUBE CORNERS,

      return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ),  // AND ADD
                                     grad(p[BA  ], x-1, y  , z   )), // BLENDED
                             lerp(u, grad(p[AB  ], x  , y-1, z   ),  // RESULTS
                                     grad(p[BB  ], x-1, y-1, z   ))),// FROM  8
                     lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ),  // CORNERS
                                     grad(p[BA+1], x-1, y  , z-1 )), // OF CUBE
                             lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                     grad(p[BB+1], x-1, y-1, z-1 ))));
   }
   static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
   static double lerp(double t, double a, double b) { return a + t * (b - a); }
   static double grad(int hash, double x, double y, double z) {
      int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
      double u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
             v = h<4 ? y : h==12||h==14 ? x : z;
      return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
   }
};


};

#endif // GRINLIZ_NOISE_INCLUDED

