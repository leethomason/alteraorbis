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


#ifndef GRINLIZ_VECTOR_INCLUDED
#define GRINLIZ_VECTOR_INCLUDED

#include <math.h>
#include "gldebug.h"
#include "glmath.h"
#include "glutil.h"


namespace grinliz {
		
// Surprisingly tricky and troublesome function.
template< class T, int COMP >
inline void Normalize(T* v)
{
	T len2 = 0;
	for (int i = 0; i < COMP; ++i) {
		len2 += v[i] * v[i];
	}
	if (len2 == 0) {
		// Set x to 1, the rest to zero.
		v[0] = 1;
		for (int i = 1; i < COMP; ++i)
			v[i] = 0;
	}
	else {
		T lenInv = static_cast<T>(1) / sqrt(len2);
		for (int i = 0; i<COMP; ++i)
			v[i] *= lenInv;

#ifdef DEBUG
		// Did this work?
		T lenCheck2 = 0;
		for (int i = 0; i<COMP; ++i) {
			lenCheck2 += v[i] * v[i];
		}
		GLASSERT(lenCheck2 > static_cast<T>(.99) && lenCheck2 < static_cast<T>(1.01));
#endif
	}
}

template< class T >
struct Vector2
{
	enum { COMPONENTS = 2 };

	T x;
	T y;

	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Add( const Vector2<T>& vec ) {
		x += vec.x;
		y += vec.y;
	}

	void Subtract( const Vector2<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
	}

	Vector2<T> operator-() const { Vector2<T> vec = { -x, -y }; return vec; }

	friend Vector2<T> operator-( const Vector2<T>& head, const Vector2<T>& tail ) {
		Vector2<T> vec = {
			head.x - tail.x,
			head.y - tail.y,
		};
		return vec;
	}

	friend Vector2<T> operator+( const Vector2<T>& head, const Vector2<T>& tail ) {
		Vector2<T> vec = {
			head.x + tail.x,
			head.y + tail.y,
		};
		return vec;
	}
		
	friend Vector2<T> operator*( const Vector2<T>& head, T scalar ) { Vector2<T> r = { head.x*scalar, head.y*scalar }; return r; }
	friend Vector2<T> operator*( T scalar, const Vector2<T>& head ) { Vector2<T> r = { head.x*scalar, head.y*scalar }; return r; }

	void operator+=( const Vector2<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector2<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector2<T>& rhs ) const	{ return (x == rhs.x) && (y == rhs.y); }
	bool operator!=( const Vector2<T>& rhs ) const	{ return (x != rhs.x) || (y != rhs.y); }

	// This is used mainly for sorting; no geometric meaning.
	bool operator<(const Vector2<T>& rhs) const {
		if (y < rhs.y) return true;
		if (y > rhs.y) return false;
		return x < rhs.x;
	}

	bool Equal( const Vector2<T>& b, T epsilon ) const {
		return ( grinliz::Equal( x, b.x, epsilon ) && grinliz::Equal( y, b.y, epsilon ) );
	}

	int Compare( const Vector2<T>& r, T epsilon ) const
	{
		if ( Equal( this->x, r.x, epsilon ) ) {
			if ( Equal( this->y, r.y, epsilon ) ) {
				return 0;
			}
			else if ( this->y < r.y ) return -1;
			else					  return 1;
		}
		else if ( this->x < r.x ) return -1;
		else					  return 1;
	}	

	void Set( T _x, T _y ) {
		this->x = _x;
		this->y = _y;
	}

	void Zero() {
		x = (T)0;
		y = (T)0;
	}

	bool IsZero()  const { 
		return x == 0 && y == 0;
	}

	T Length() const { return grinliz::Length( x, y ); };
	T LengthSquared() const { return x*x + y*y; }

	friend T Length( const Vector2<T>& a, const Vector2<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y );
	}

	void Normalize()
	{
		grinliz::Normalize<T, COMPONENTS>(&x);
	}

	void RotatePos90()
	{
		T a = x;
		T b = y;
		x = -b;
		y = a;
	}
	void RotateNeg90()
	{
		T a = x;
		T b = y;
		x = b;
		y = -a;
	}

	// Assuming 'this' and 'rhs' are points, are they
	// adjacent points? (One unit apart.)
	bool IsAdjacent(const Vector2<T>& rhs, bool includeDiagonals) const {
		int dx = abs(T(rhs.x - x));
		int dy = abs(T(rhs.y - y));

		if (dx == 0 && dy == 0)
			return false;

		if (includeDiagonals) {
			return dx <= 1 && dy <= 1;
		}
		return (dx + dy) == 1;
	}

	// Assuming 'this' is a point, get the adjacent ones.
	Vector2<T> Adjacent(int which) const {
		GLASSERT(which >= 0 && which < 4);
		static const Vector2<T> delta[4] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
		return *this + delta[which];
	}
};

typedef Vector2< int > Vector2I;
typedef Vector2< float > Vector2F;


inline void DegreeToVector( float degree, Vector2<float>* vec )
{
	float rad = ToRadian( degree );
	vec->x = cosf( rad );
	vec->y = sinf( rad );
}


template< class T >
struct Line2
{
	Vector2<T>	n;	// normal
	T			d;		// offset

	T EvaluateX( T y )		{ return ( -d - y*n.y ) / n.x; }
	T EvaluateY( T x )		{ return ( -d - x*n.x ) / n.y; }
};

typedef Line2< float > Line2F;



template< class T >
struct Vector3
{
	enum { COMPONENTS = 3 };

	T x;
	T y;
	T z;

	T& r() { return x; }
	T& g() { return y; }
	T& b() { return z; }

	T r() const { return x; }
	T g() const { return y; }
	T b() const { return z; }

	// I avoid non-const references - but this one is just so handy! And, in
	// use in the code, it is clear it is an assignment.
	
	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Zero() { x = y = z = 0; }

	bool IsZero()  const { 
		return x == 0 && y == 0 && z == 0;
	}


	void Add( const Vector3<T>& vec ) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
	}

	friend void Add( const Vector3<T>& a, const Vector3<T>& b, Vector3<T>* c )
	{
		c->x = a.x + b.x;
		c->y = a.y + b.y;
		c->z = a.z + b.z;
	}

	void Subtract( const Vector3<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}

	friend Vector3<T> operator-( const Vector3<T>& head, const Vector3<T>& tail ) {
		Vector3<T> vec = {
			head.x - tail.x,
			head.y - tail.y,
			head.z - tail.z
		};
		return vec;
	}

	friend Vector3<T> operator+( const Vector3<T>& head, const Vector3<T>& tail ) {
		Vector3<T> vec = {
			head.x + tail.x,
			head.y + tail.y,
			head.z + tail.z,
		};
		return vec;
	}
	
	friend Vector3<T> operator*( const Vector3<T>& head, T scalar ) { Vector3<T> r = head; r.Multiply( scalar ); return r; }
	friend Vector3<T> operator*( T scalar, const Vector3<T>& head ) { Vector3<T> r = head; r.Multiply( scalar ); return r; }

	Vector3<T> operator-() const {
		Vector3<T> vec = { -x, -y, -z };
		return vec;
	}

	void operator+=( const Vector3<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector3<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector3<T>& rhs ) const	{ return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=( const Vector3<T>& rhs ) const	{ return x != rhs.x || y != rhs.y || z != rhs.z; }

	bool Equal( const Vector3<T>& b, T epsilon ) const {
		return ( grinliz::Equal( x, b.x, epsilon ) && grinliz::Equal( y, b.y, epsilon ) && grinliz::Equal( z, b.z, epsilon ) );
	}

	friend bool Equal( const Vector3<T>& a, const Vector3<T>& b, T epsilon=0.001f )  {
		return ( grinliz::Equal( a.x, b.x, epsilon ) && grinliz::Equal( a.y, b.y, epsilon ) && grinliz::Equal( a.z, b.z, epsilon ) );
	}

	int Compare( const Vector3<T>& r, T epsilon ) const
	{
		if ( Equal( this->x, r.x, epsilon ) ) {
			if ( Equal( this->y, r.y, epsilon ) ) {
				if ( Equal( this->z, r.z, epsilon ) ) {
					return 0;
				}
				else if ( this->z < r.z ) return -1;
				else					  return 1;
			}
			else if ( this->y < r.y ) return -1;
			else					  return 1;
		}
		else if ( this->x < r.x ) return -1;
		else					  return 1;
	}

	void Set( T _x, T _y, T _z ) {
		this->x = _x;
		this->y = _y;
		this->z = _z;
	}

	void XZ( const Vector2<T>& v ) {
		this->x = v.x;
		this->z = v.y;
	}

	Vector2<T> XZ() {
		Vector2<T> xz = { x, z }; 
		return xz;
	}

	void Normalize()	
	{
		grinliz::Normalize<T, COMPONENTS>(&x);
	}

	T Length() const		{ return grinliz::Length( x, y, z ); };
	T LengthSquared() const { return x*x + y*y + z*z; }

	friend T Length( const Vector3<T>& a, const Vector3<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y, a.z-b.z );
	}

	static void Lerp(const Vector3<T>& a, const Vector3<T>& b, T fraction, Vector3<T>* out) {
		out->x = grinliz::Lerp(a.x, b.x, fraction);
		out->y = grinliz::Lerp(a.y, b.y, fraction);
		out->z = grinliz::Lerp(a.z, b.z, fraction);
	}

	#ifdef DEBUG
	void Dump( const char* name ) const
	{
		GLOUTPUT(( "Vec%4s: %6.2f %6.2f %6.2f\n", name, (float)x, (float)y, (float)z ));
	}
	#endif
};


inline void DegreeToVector( float degree, Vector3<float>* vec )
{
	Vector2<float> v;
	DegreeToVector( degree, &v );
	vec->Set( v.x, v.y, 0.0f );
}

template< class T >
inline void Convert( const Vector3<T>& v3, Vector2<T>* v2 )
{
	v2->x = v3.x;
	v2->y = v3.y;
}


template< class T >
struct Vector4
{
	enum { COMPONENTS = 4 };

	T x;
	T y;
	T z;
	T w;

	T& r() { return x; }
	T& g() { return y; }
	T& b() { return z; }
	T& a() { return w; }

	T r() const { return x; }
	T g() const { return y; }
	T b() const { return z; }
	T a() const { return w; }

	// I avoid non-const references - but this one is just so handy! And, in
	// use in the code, it is clear it is an assignment.
	T& X( int i )    		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }
	T  X( int i ) const		{	GLASSERT( InRange( i, 0, COMPONENTS-1 ));
								return *( &x + i ); }

	void Add( const Vector4<T>& vec ) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
	}

	friend void Add( const Vector4<T>& a, const Vector4<T>& b, Vector4<T>* c )
	{
		c->x = a.x + b.x;
		c->y = a.y + b.y;
		c->z = a.z + b.z;
		c->w = a.w + b.w;
	}

	void Subtract( const Vector4<T>& vec ) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
	}

	void Multiply( T scalar ) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
	}

	friend Vector4<T> operator-( const Vector4<T>& head, const Vector4<T>& tail ) {
		Vector4<T> vec = {
			head.x - tail.x,
			head.y - tail.y,
			head.z - tail.z,
			head.w - tail.w
		};
		return vec;
	}

	friend Vector4<T> operator+( const Vector4<T>& head, const Vector4<T>& tail ) {
		Vector4<T> vec = {
			head.x + tail.x,
			head.y + tail.y,
			head.z + tail.z,
			head.w + tail.w,
		};
		return vec;
	}
	
	friend Vector4<T> operator*( const Vector4<T>& head, T scalar ) { Vector4<T> r = head; r.Multiply( scalar ); return r; }
	friend Vector4<T> operator*( T scalar, const Vector4<T>& head ) { Vector4<T> r = head; r.Multiply( scalar ); return r; }
	friend Vector4<T> operator*( const Vector4<T>&a, const Vector4<T>& b ) { Vector4<T> r = { a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w }; return r; }

	Vector4<T> operator-() const {
		Vector4<T> vec = { -x, -y, -z, -w };
		return vec;
	}

	void operator+=( const Vector4<T>& vec )		{ Add( vec ); }
	void operator-=( const Vector4<T>& vec )		{ Subtract( vec ); }
	bool operator==( const Vector4<T>& rhs ) const	{ return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=( const Vector4<T>& rhs ) const	{ return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

	void Set( T _x, T _y, T _z, T _w ) {
		this->x = _x;
		this->y = _y;
		this->z = _z;
		this->w = _w;
	}

	void Set( const Vector3<T>& v, T _w ) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = _w;
	}

	void Zero() {
		x = (T)0;
		y = (T)0;
		z = (T)0;
		w = (T)0;
	}

	void Normalize()	
	{ 
		grinliz::Normalize<T, COMPONENTS>(&x);
	}

	T Length() const { return grinliz::Length( x, y, z, w ); };

	friend T Length( const Vector4<T>& a, const Vector4<T>& b ) {
		return grinliz::Length( a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w );
	}
};


typedef Vector3< int > Vector3I;
typedef Vector3< float > Vector3F;

typedef Vector4< int > Vector4I;
typedef Vector4< float > Vector4F;

static const Vector3F V3F_ZERO = { 0, 0, 0 };
static const Vector4F V4F_ZERO = { 0, 0, 0, 0 };

};	// namespace grinliz

#endif
