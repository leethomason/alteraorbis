/*
Copyright (c) 2000-2015 Lee Thomason (www.grinninglizard.com)
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


#ifndef GRINLIZ_MATRIX_DEFINED
#define GRINLIZ_MATRIX_DEFINED

#include "gldebug.h"
#include "glvector.h"
#include "glmath.h"
#include "glrectangle.h"
#include <memory.h>

#ifdef DEBUG
//#define MAT_DEBUG_DEEP
#endif

namespace grinliz {

/** A 3D homogenous matrix. Although the bottom row is typically 0,0,0,1 this
	implementation uses the full 4x4 computation.

	It uses the (unfortutate) OpenGL layout:
	@verbatim
       (  m[0]   m[4]    m[8]   m[12] )    ( v[0])
        | m[1]   m[5]    m[9]   m[13] |    | v[1]|
 M(v) = | m[2]   m[6]   m[10]   m[14] |  x | v[2]|
       (  m[3]   m[7]   m[11]   m[15] )    ( v[3])
	@endverbatim
*/
class Matrix4
{
  public:
	enum { COMPONENTS = 4 };
	enum {
		T_TERM = 0x01,
		R_TERM = 0x02,
		P_TERM = 0x04,

		UNKNOWN_TYPE = 0xff,
		IDENTITY_TYPE = 0
	};
	enum {
		M11, M21, M31, M41, 
		M12, M22, M32, M42,
		M13, M23, M33, M43,
		M14, M24, M34, M44
	};

	inline static int INDEX( int row, int col ) {	GLASSERT( row >= 0 && row < 4 );
													GLASSERT( col >= 0 && col < 4 );
													return col*4+row; }

	/// Construct an identity matrix
	Matrix4()							{ type = IDENTITY_TYPE;  SetIdentity(); }
	Matrix4(const Matrix4& rhs)			{ type = rhs.type; for (int i = 0; i < COMPONENTS*COMPONENTS; ++i) x[i] = rhs.x[i]; }
	void operator=(const Matrix4& rhs)	{ type = rhs.type; for (int i = 0; i < COMPONENTS*COMPONENTS; ++i) x[i] = rhs.x[i]; }

	/// Set the matrix to identity
	void SetIdentity()		{	x[0] = x[5] = x[10] = x[15] = 1.0f;
								x[1] = x[2] = x[3] = x[4] = x[6] = x[7] = x[8] = x[9] = x[11] = x[12] = x[13] = x[14] = 0.0f; 
								type = IDENTITY_TYPE;
							}

	void Set(int row, int col, float v)		{ x[INDEX(row, col)] = v; type = UNKNOWN_TYPE; }
	float& m(int row, int col)				{ return x[INDEX(row, col)]; type = UNKNOWN_TYPE; }
	float m(int row, int col ) const		{	return x[INDEX(row,col)]; }
	float& X(int index)						{
		GLASSERT(index >= 0 && index < 16);
		type = UNKNOWN_TYPE;
		return x[index];
	}
	float X(int index) const 	{
		GLASSERT(index >= 0 && index < 16);
		return x[index];
	}
	const float* Mem() const { return x; }
	float* Mem() { type = UNKNOWN_TYPE;  return x; }

	/// Set the translation terms
	void SetTranslation(float _x, float _y, float _z)		{ GLASSERT(IsIdentity()); type = UNKNOWN_TYPE; x[12] = _x;	x[13] = _y;	x[14] = _z; }
	/// Set the translation terms
	void SetTranslation(const Vector3F& vec)				{ GLASSERT(IsIdentity()); type = UNKNOWN_TYPE; x[12] = vec.x;	x[13] = vec.y;	x[14] = vec.z; }

	/// Set the rotation terms
	void SetXRotation( float thetaDegree );
	/// Set the rotation terms
	void SetYRotation( float thetaDegree );
	/// Set the rotation terms
	void SetZRotation( float thetaDegree );
	/// Concatenate a rotation in.
	void ConcatRotation( float thetaDegree, int axis );
	void ConcatTranslation(const Vector3F& t);

	/// Get the rotation around the X(0), Y(1), or Z(2) axis
	float CalcRotationAroundAxis( int axis ) const;

	/// Set the scale terms
	void SetScale(float scale)	{ GLASSERT(IsIdentity()); type = UNKNOWN_TYPE; x[0] = x[5] = x[10] = scale; }

	/// Set the matrix from an axis-angle representation
	void SetAxisAngle( const Vector3F& axis, float angle );

	/// Set this to be an (OpenGL) frustum perspective matrix.
	void SetFrustum( float left, float right, float bottom, float top, float near, float far );
	/// Set this to be an (OpenGL) ortho perspective matrix.
	void SetOrtho( float left, float right, float bottom, float top, float zNear, float zFar );

	void StripTrans()		{	x[12] = x[13] = x[14] = 0.0f;
								x[15] = 1.0f;
								type = UNKNOWN_TYPE;
							}

	/// Is this probably a rotation matrix?
	bool IsRotation() const;

	/// Return a row of the matrix
	Vector3F Row( int i ) const	{
		Vector3F r = { x[INDEX(i,0)], x[INDEX(i,1)], x[INDEX(i,2)] };
		return r;
	}
	/// Return a column of the matrix
	Vector3F Col( int i ) const	{ 
		Vector3F c = { x[INDEX(0,i)], x[INDEX(1,i)], x[INDEX(2,i)] };
		return c;
	}

	void SetRow( int i, const Vector4F& v ) {
		x[INDEX(i,0)] = v.x;
		x[INDEX(i,1)] = v.y;
		x[INDEX(i,2)] = v.z;
		x[INDEX(i,3)] = v.w;
		type = UNKNOWN_TYPE;
	}

	void SetRow( int i, float _x, float y, float z, float w ) {
		x[INDEX(i,0)] = _x;
		x[INDEX(i,1)] = y;
		x[INDEX(i,2)] = z;
		x[INDEX(i,3)] = w;
		type = UNKNOWN_TYPE;
	}

	void SetCol( int i, const Vector4F& v ) {
		x[INDEX(0,i)] = v.x;
		x[INDEX(1,i)] = v.y;
		x[INDEX(2,i)] = v.z;
		x[INDEX(3,i)] = v.w;
		type = UNKNOWN_TYPE;
	}

	void SetCol( int i, float _x, float y, float z, float w ) {
		x[INDEX(0,i)] = _x;
		x[INDEX(1,i)] = y;
		x[INDEX(2,i)] = z;
		x[INDEX(3,i)] = w;
		type = UNKNOWN_TYPE;
	}

	/// Transpose 
	void Transpose( Matrix4* transpose ) const;
	void Transpose();
	/// Determinant
	float Determinant() const;
	/// Adjoint
	void Adjoint( Matrix4* adjoint ) const;
	/// Invert
	void Invert( Matrix4* inverse ) const;
	/// Inverte, assuming this is only rotation and translation. Results will be incorrect for scale or skew.
	void InvertRotationMat( Matrix4* inverse ) const;
	/// Cofactor
	void Cofactor( Matrix4* cofactor ) const;

	float SubDeterminant(int excludeRow, int excludeCol) const;

	inline float Determinant2x2(	float a, float b, 
									float c, float d ) const {
		return a*d - c*b;
	}

	inline float Determinant3x3(	float a1, float a2, float a3,
									float b1, float b2, float b3,
									float c1, float c2, float c3 ) const 
	{
		return a1 * Determinant2x2( b2, b3, c2, c3 ) -
			   b1 * Determinant2x2( a2, a3, c2, c3 ) +
			   c1 * Determinant2x2( a2, a3, b2, b3 );
	}


	/// Multiply all the terms of the matrix
	void ApplyScalar( float v ) {
		for( int i=0; i<16; ++i )
			x[i] *= v;
		type = UNKNOWN_TYPE;
	}


	bool IsIdentity() const {
		return type == IDENTITY_TYPE;
	}

	friend void MultMatrix4( const Matrix4& a, const Matrix4& b, Matrix4* c );
	friend void MultMatrix4( const Matrix4& a, const Vector3F& b, Vector3F* c, float w );
	friend void MultMatrix4( const Matrix4& a, const Vector4F& b, Vector4F* c );
	friend void MultMatrix4( const Matrix4& m, const Rectangle3<float>& in, Rectangle3<float>* out );
	
	void Dump( const char* name=0 ) const
	{
		const char* empty = "";
		if ( !name ) {
			name = empty;
		}
		GLOUTPUT(( "Mat%4s: %7.3f %7.3f %7.3f %7.3f\n"
				  "         %7.3f %7.3f %7.3f %7.3f\n"			
				  "         %7.3f %7.3f %7.3f %7.3f\n"			
				  "         %7.3f %7.3f %7.3f %7.3f\n",
				  name,
				  x[0], x[4], x[8], x[12], 
				  x[1], x[5], x[9], x[13], 			
				  x[2], x[6], x[10], x[14], 
				  x[3], x[7], x[11], x[15] ));
	}

	static void Test();
	
	friend Matrix4 operator*( const Matrix4& a, const Matrix4& b )
	{	
		Matrix4 result;
		MultMatrix4( a, b, &result );
		return result;
	}
	friend Vector3F operator*( const Matrix4& a, const Vector3F& b )
	{
		Vector3F result;
		MultMatrix4( a, b, &result, 1.0f );
		return result;
	}
	friend Vector4F operator*( const Matrix4& a, const Vector4F& b )
	{
		Vector4F result;
		MultMatrix4( a, b, &result );
		return result;
	}

	friend bool Equal( const Matrix4& a, const Matrix4& b, float epsilon = 0.001f )
	{
		for( unsigned i=0; i<16; ++i )
			if ( !grinliz::Equal( a.x[i], b.x[i], epsilon ) )
				return false;
		return true;
	}
	int GetType() const;

private:
	bool operator==(const Matrix4& rhs) const;
	bool operator!=(const Matrix4& rhs) const;
	static void MultMatrix4Expanded( const Matrix4& a, const Matrix4& b, Matrix4* c );

	bool HasTTerm() const;
	bool HasRTerm() const;
	bool HasPTerm() const;

	// Row-Column notation is backwards from x,y regrettably. Very
	// confusing. Just uses array. Increment by one moves down to the next
	// row, so that the next columnt is at +4.
	float x[16];
	mutable int type;
};


/** Compute the world to eye transform. Given a camera in world coordinates, 
	what is the world to eye transform?
	This transform should be applied to the MODELVIEW matrix. Any of the
	output params can be null.

	@param eye		location to look from
	@param center	location to look to
	@param up		defines the up vector 
*/
void LookAt( bool cameraFlipBug,
			 const Vector3F& eye, const Vector3F& center, const Vector3F& up,
			 Matrix4* rotation, Matrix4* translation, Matrix4* view );

/** C = AB. Perform the operation in column-major form, meaning a vector as a columns.
	Note the target parameter is the last parameter, although it is more
	comfortable expressed: C = AB
*/
void MultMatrix4( const Matrix4& a, const Matrix4& b, Matrix4* c );

/** out = Mv. Multiply a vector by a matrix. A vector is a column in column-major form.
	w is the v.w term. If 1.0 (default) transformation as point. If 0.0 transform
	as a direction.
*/
inline void MultMatrix4( const Matrix4& m, const Vector3F& v, Vector3F* out, float w )
{
	GLASSERT( out != &v );
	for( int i=0; i<3; ++i )			// target row
	{
		*( &out->x + i )	=		m.x[i+0]  * v.x 
								  + m.x[i+4]  * v.y 
								  + m.x[i+8]  * v.z
								  + m.x[i+12] * w;
	}
}

inline void MultMatrix4( const Matrix4& m, const Vector4F& v, Vector4F* w )
{
	GLASSERT( w != &v );
	for( int i=0; i<4; ++i )			// target row
	{
		*( &w->x + i )	=		m.x[i+0]  * v.x 
							  + m.x[i+4]  * v.y 
							  + m.x[i+8]  * v.z
							  + m.x[i+12] * v.w;
	}
}

void MultMatrix4( const Matrix4& m, const Rectangle3F& in, Rectangle3F* out );



class Matrix2
{
  public:
	enum { COMPONENTS = 2 };

	inline static int INDEX( int row, int col ) {	GLASSERT( row >= 0 && row < COMPONENTS );
													GLASSERT( col >= 0 && col < COMPONENTS );
													return col*COMPONENTS+row; }

	/// Construct an identity matrix
	Matrix2()								{	SetIdentity();	}
	Matrix2( const Matrix2& rhs )			{	for( int i=0; i<COMPONENTS*COMPONENTS; ++i ) x[i] = rhs.x[i]; }
	void operator=( const Matrix2& rhs )	{	for( int i=0; i<COMPONENTS*COMPONENTS; ++i ) x[i] = rhs.x[i]; }

	/// Set the matrix to identity
	void SetIdentity()		{	x[0] = x[3] = 1.0f;
								x[1] = x[2] = 0.0f; 
							}

	void Set( int row, int col, float v )	{	x[INDEX(row,col)] = v; }
	float& m( int row, int col )			{	return x[INDEX(row,col)]; }
	float m(int row, int col ) const		{	return x[INDEX(row,col)]; }

	/// Set the rotation terms
	void SetRotation( float thetaDegree );

	bool operator==( const Matrix2& rhs ) const	{ 
		for( int i=0; i<COMPONENTS*COMPONENTS; ++i )
			if ( x[i] != rhs.x[i] )
				return false;
		return true;
	}

	bool operator!=( const Matrix2& rhs ) const	{ 
		int match = 0;
		for( int i=0; i<COMPONENTS*COMPONENTS; ++i )
			if ( x[i] == rhs.x[i] )
				++match;
		if ( match == COMPONENTS*COMPONENTS ) return false;
		return true;
	}

	/// Invert
	void Invert( Matrix2* s ) const
	{
		float det = a*d - b*c;
		float inv = 1.0f / det;
		s->a = d * inv;
		s->b = -b * inv;
		s->c = -c * inv;
		s->d = a * inv;
	}


	bool IsIdentity() const {
		return    x[0] == 1 && x[3] == 1 
			   && x[1] == 0 && x[2] == 0;
	}
	
#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable : 4201 )	// un-named union.
#endif
	// Row-Column notation is backwards from x,y regrettably. Very
	// confusing. Just uses array. Increment by one moves down to the next
	// row, so that the next columnt is at +4.
	union
	{
		float x[COMPONENTS*COMPONENTS];
		struct {
			// row-column
			float m11, m21, m12, m22;
		};
		struct {
			float a, c, b, d;	// why I hate this memory arrangement, illustrated.
		};
	};
#ifdef _MSC_VER
#pragma warning ( pop )
#endif

	friend Matrix2 operator*( const Matrix2& a, const Matrix2& b )
	{	
		Matrix2 r;
		r.m11 = a.m11*b.m11 + a.m12*b.m21;
		r.m12 = a.m11*b.m12 + a.m12*b.m22;
		r.m21 = a.m21*b.m11 + a.m22*b.m21;
		r.m22 = a.m21*b.m12 + a.m22*b.m22;
		return r;
	}

	friend Vector2<float> operator*( const Matrix2& a, const Vector2<float>& b )
	{
		Vector2F r;
		r.x = a.m11*b.x + a.m12*b.y;
		r.y = a.m21*b.x + a.m22*b.y;
		return r;
	}
};


}// namespace grinliz

#endif
