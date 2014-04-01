/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/


#ifndef GRINLIZ_MATH_INCLUDED
#define GRINLIZ_MATH_INCLUDED

#include "glutil.h"

namespace grinliz
{
const float  PI = 3.1415926535897932384626433832795f;
const float  TWO_PI = 2.0f*3.1415926535897932384626433832795f;
const double PI_D = 3.1415926535897932384626433832795;
const double TWO_PI_D = 2.0*3.1415926535897932384626433832795;

const float RAD_TO_DEG = (float)( 360.0 / ( 2.0 * PI ) );
const float DEG_TO_RAD = (float)( ( 2.0 * PI ) / 360.0f );
const double RAD_TO_DEG_D = ( 360.0 / ( 2.0 * PI_D ) );
const double DEG_TO_RAD_D = ( ( 2.0 * PI_D ) / 360.0 );
const float SQRT2 = 1.4142135623730950488016887242097f;
const float SQRT2OVER2 = float( 1.4142135623730950488016887242097 / 2.0 );

inline float ToDegree( float radian ) { return radian * RAD_TO_DEG; }
inline double ToDegree( double radian ) { return radian * RAD_TO_DEG_D; }
inline float ToRadian( float degree ) { return degree * DEG_TO_RAD; }
inline double ToRadian( double degree ) { return degree * DEG_TO_RAD_D; }

void SinCosDegree( float degreeTheta, float* sinTheta, float* cosTheta );

const float EPSILON = 0.000001f;

inline float NormalizeAngleDegrees( float alpha ) {
	GLASSERT( alpha >= -360.f && alpha <= 720.0f );	// check for reasonable input
	while ( alpha < 0.0f )		alpha += 360.0f;
	while ( alpha >= 360.0f )	alpha -= 360.0f;
	return alpha;
}

inline int NormalizeAngleDegrees( int alpha ) {
	GLASSERT( alpha >= -360 && alpha <= 720 );
	while ( alpha < 0 )		alpha += 360;
	while ( alpha >= 360 )	alpha -= 360;
	return alpha;
}


/** A loose equality check. */
inline bool Equal( float x, float y, float epsilon )
{
	return fabsf( x - y ) <= epsilon;
}

inline bool Equal( double x, double y, double epsilon )
{
	return fabs( x - y ) <= epsilon;
}

inline bool EqualInt( float a, float epsilon = EPSILON )
{
	return Equal( (float)LRint(a), a, epsilon );
}

/** The shortest path between 2 angles. (AngleBetween)
	@param angle0	The first angle in degrees.
	@param angle1	The second angle in degrees.
	@param bias		The direction of travel - either +1.0 or -1.0
	@return distance	The distance in degrees between the 2 angles - always positive.
*/
float MinDeltaDegrees( float angle0, float angle1, float* bias );

inline float RotationXZDegrees( float x, float z ) {
	return NormalizeAngleDegrees( ToDegree( atan2f( x, z )));
}


inline float SquareF( float x ) { return x*x; }

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y )
{
	// It's worth some time here:
	//		http://www.azillionmonkeys.com/qed/sqroot.html
	// to make this (perhaps) better. 

	// Use the hopefully faster "f" version.
	return sqrtf( x*x + y*y );
}

inline double Length( double x, double y ) 
{ 
	// The more accurate double version.
	return sqrt( x*x + y*y ); 
}

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y, float z )
{
	return sqrtf( x*x + y*y + z*z );
}

inline double Length( double x, double y, double z ) 
{ 
	return sqrt( x*x + y*y + z*z ); 
}

/** A length that is reasonably accurate. (24 bits or better.) */
inline float Length( float x, float y, float z, float w )
{
	float len2 = x*x + y*y + z*z + w*w;
	return len2 == 0 ? 0 :sqrtf( x*x + y*y + z*z + w*w );
}


/// Template to return the average of 2 numbers.
template <class T> inline T Average( T y0, T y1 )
{
	return ( y0 + y1 ) / T( 2 );
}


template< class T, int SIZE >
class MathVector
{
public:
	MathVector() { Zero(); }

	void Zero()				{ for ( int i=0; i<SIZE; ++i ) vec[i] = 0; }
	void Mult( float m )	{ for ( int i=0; i<SIZE; ++i ) vec[i] *= m; }

	T& operator[](int i)				{ GLASSERT( i>=0 && i<SIZE); return vec[i]; }
	const T& operator[](int i) const	{ GLASSERT( i>=0 && i<SIZE); return vec[i]; }

	T vec[SIZE];
};


template <class T> inline T Sign( T x ) {
	if ( x > 0 ) return 1;
	if ( x < 0 ) return -1;
	return 0;
}

};	// namespace grinliz

#endif
