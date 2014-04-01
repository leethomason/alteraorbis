/*
Copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)
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



#ifndef GRINLIZ_UTIL_INCLUDED
#define GRINLIZ_UTIL_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <math.h>
#include <stdlib.h>
#include "gldebug.h"
#include "gltypes.h"

namespace grinliz {

// Sleazy C trick that looks weird in code.
#define GL_C_ARRAY_SIZE( a ) (sizeof(a)/sizeof(a[0]))

/// Minimum
template <class T> inline T		Min( T a, T b )		{ return ( a < b ) ? a : b; }
/// Maximum
template <class T> inline T		Max( T a, T b )		{ return ( a > b ) ? a : b; }
/// Minimum
template <class T> inline T		Min( T a, T b, T c )	{ return Min( a, Min( b, c ) ); }
/// Maximum
template <class T> inline T		Max( T a, T b, T c )	{ return Max( a, Max( b, c ) ); }
/// Minimum
template <class T> inline T		Min( T a, T b, T c, T d )	{ return Min( d, Min( a, b, c ) ); }
/// Maximum
template <class T> inline T		Max( T a, T b, T c, T d )	{ return Max( d, Max( a, b, c ) ); }

/// Swap
template <class T> inline void	Swap( T* a, T* b )	{ T temp; temp = *a; *a = *b; *b = temp; }
/// Returns true if value in the range [lower, upper]
template <class T> inline bool	InRange( T a, T lower, T upper )	{ return a >= lower && a <= upper; }
/// Returned the value clamped to the range [lower, upper]
template <class T> inline T		Clamp( const T& a, T lower, T upper )	
								{ 
									if ( a < lower )
										return lower;
									else if ( a > upper )
										return upper;
									return a;
								}

/// Average (mean) value
template <class T> inline T		Mean( T t0, T t1 )	{ return (t0 + t1)/static_cast<T>( 2 ); }
/// Average (mean) value
template <class T> inline T		Mean( T t0, T t1, T t2 )	{ return (t0+t1+t2)/static_cast<T>( 3 ); }

/// Find the highest bit set.
inline U32 LogBase2( U32 v ) 
{
	// I don't love this implementation, and I
	// don't love the table alternatives either.
	U32 r=0;
	while ( v >>= 1 ) {
		++r;
	}
	return r;
}

/// Round down to the next power of 2
inline U32 FloorPowerOf2( U32 v )
{
	v = v | (v>>1);
	v = v | (v>>2);
	v = v | (v>>4);
	v = v | (v>>8);
	v = v | (v>>16);
	return v - (v>>1);
}

/// Round up to the next power of 2
inline U32 CeilPowerOf2( U32 v )
{
	v = v - 1;
	v = v | (v>>1);
	v = v | (v>>2);
	v = v | (v>>4);
	v = v | (v>>8);
	v = v | (v>>16);
	return v + 1;
}

inline bool IsPowerOf2( U32 x ) 
{
	return x == CeilPowerOf2( x );
}

/// Linear interpolation.
template <class A, class B> inline B Interpolate( A x0, B q0, A x1, B q1, A x )
{
	GLASSERT( static_cast<B>( x1 - x0 ) != 0.0f );
	return q0 + static_cast<B>( x - x0 ) * ( q1 - q0 ) / static_cast<B>( x1 - x0 );
}


// More useful versions of the "HermiteInterpolate" and "InterpolateUnitX"
template <class T>
inline T Lerp( const T& x0, const T& x1, float t )
{
	return  x0 + (x1-x0)*t;
}


template <class T>
inline T Lerp( const T& x0, const T& x1, double t )
{
	return  x0 + (x1-x0)*t;
}


// Cubic fade (hermite)
// Use: Lerp( a, b, Fade3( t ));
inline float Fade3( float t )
{
	return t*t*(3.0f-2.0f*t);
}


// Fade with a 5th order polynomial
// Use: Lerp( a, b, Fade5( t ));
inline float Fade5( float t )
{ 
	return t*t*t*(t*(t*6.f - 15.f) + 10.f); 
}


/** Template function for linear interpolation between 4 points, at x=0, x=1, y=0, and y=1.
*/
template <class T> inline T BilinearInterpolate( T q00, T q10, T q01, T q11, T x, T y )
{
	const T one = static_cast<T>(1);
	return q00*(one-x)*(one-y) + q10*(x)*(one-y) + q01*(one-x)*(y) + q11*(x)*(y);
}


/** Template function for linear interpolation between 4 points, at x=0, x=1, y=0, and y=1.
	Adds a weight factor for each point. A weight can be 0, but not all weights should be
	0 (if they are, 0 will be returned).
	The order of points in the input array is q00, q10, q01, q11.
*/
template <class T> inline T BilinearInterpolate( const T* q, T x, T y, const T* weight )
{
	const T one = static_cast<T>(1);
	const T zero = static_cast<T>(0);
	const T area[4] = { (one-x)*(one-y), (one-x)*(y), (x)*(one-y), x*y };

	T totalWeight = zero;
	T sum = zero;

	for ( unsigned i=0; i<4; ++i ) {
		totalWeight += area[i] * weight[i];
		sum += q[i] * area[i] * weight[i];
	}
	if ( totalWeight == zero )
		return zero;
	return sum / totalWeight;
}


/// fast double to long conversion, with rounding.
inline long LRint( double val)
{
	#if defined (__GNUC__)
		return lrint( val );
	#elif defined (_MSC_VER)
		long retval;
		__asm fld val
		__asm fistp retval
		return retval;
	#else
		// This could be better. 
		return (long)( val + 0.5 );
	#endif
}


/// fast float to long conversion, with rounding.
inline long LRint( float val)
{
	#if defined (__GNUC__)
		return lrintf( val );
	#elif defined (_MSC_VER)
		long retval;
		__asm fld val
		__asm fistp retval
		return retval;
	#else
		// This could be better. 
		return (long)( val + 0.5f );
	#endif
}


inline long LRintf(float val) { return LRint(val); }


#if 0
/*	A class to store a set of bit flags, and set and get them in a standard way.
	Class T must be some kind of integer type.
*/
template < class T >
class Flag
{
  public:
	Flag( int initFlags )				{ store = initFlags; }
	
	inline void Set( T flag )			{ store |= flag; }
	inline void Clear( T flag )			{ store &= ~flag; }
	inline T IsSet( T flag ) const		{ return ( store & flag ); }

	inline U32  ToU32() const			{ return store; }
	inline void FromU32( U32 s )		{ store = (T) s; }

	inline void ClearAll()				{ store = 0; }

  private:
	T store;
};	
#endif



};	// namespace grinliz

#endif
