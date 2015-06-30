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

#include "glmatrix.h"
#include "glgeometry.h"
#include "glrectangle.h"

using namespace grinliz;

void grinliz::SinCosDegree(float degreeTheta, float* sinTheta, float* cosTheta)
{
	degreeTheta = NormalizeAngleDegrees(degreeTheta);

	if (degreeTheta == 0.0f) {
		*sinTheta = 0.0f;
		*cosTheta = 1.0f;
	}
	else if (degreeTheta == 90.0f) {
		*sinTheta = 1.0f;
		*cosTheta = 0.0f;
	}
	else if (degreeTheta == 180.0f) {
		*sinTheta = 0.0f;
		*cosTheta = -1.0f;
	}
	else if (degreeTheta == 270.0f) {
		*sinTheta = -1.0f;
		*cosTheta = 0.0f;
	}
	else {
		float radian = ToRadian(degreeTheta);
		*sinTheta = sinf(radian);
		*cosTheta = cosf(radian);
	}

}


void Matrix4::ConcatRotation(float thetaDegree, int axis)
{
	Matrix4 r;
	switch (axis) {
		case 0:	r.SetXRotation(thetaDegree);	break;
		case 1:	r.SetYRotation(thetaDegree);	break;
		case 2:	r.SetZRotation(thetaDegree);	break;
		default:
		GLASSERT(0);
	}
	*this = (*this) * r;
}


bool Matrix4::HasTTerm() const
{
	if (x[M14] || x[M24] || x[M34])
		return true;
	return false;
}


bool Matrix4::HasRTerm() const
{
	if (   x[M11] == 1 && x[M22] == 1 && x[M33] == 1
		&& x[M12] == 0 && x[M13] == 0 && x[M23] == 0
		&& x[M21] == 0 && x[M31] == 0 && x[M32] == 0) 
	{
		return false;
	}
	return true;
}


bool Matrix4::HasPTerm() const
{
	if (x[M41] == 0 && x[M42] == 0 && x[M43] == 0 && x[M44] == 1)
		return false;
	return true;
}


int Matrix4::GetType() const
{
	if (type == UNKNOWN_TYPE) {
		type = 0;
		if (HasTTerm())
			type |= T_TERM;
		if (HasRTerm())
			type |= R_TERM;
		if (HasPTerm())
			type |= P_TERM;
	}
#ifdef MAT_DEBUG_DEEP
	int debugType = 0;
	if (HasTTerm())
		debugType |= T_TERM;
	if (HasRTerm())
		debugType |= R_TERM;
	if (HasPTerm())
		debugType |= P_TERM;
	GLASSERT(debugType == type);
#endif
	return type;
}


/*
	RRRT
	RRRT
	RRRT
	PPPP
	*/

#define A_HAS_T(x) x
#define A_HAS_R0(x) x
#define A_HAS_R1(x) x
#define A_HAS_P0(x) x
#define A_HAS_P1(x) x
#define B_HAS_T(x) x
#define B_HAS_R0(x) x
#define B_HAS_R1(x) x
#define B_HAS_P0(x) x
#define B_HAS_P1(x) x

#define MAT_MULT_4 \
	c->x[0] = A_HAS_R1(a.x[0]) * B_HAS_R1(b.x[0]) + A_HAS_R0(a.x[4]) * B_HAS_R0(b.x[1]) + A_HAS_R0(a.x[8])  * B_HAS_R0(b.x[2]) + A_HAS_T(a.x[12])  * B_HAS_P0(b.x[3]); \
	c->x[1] = A_HAS_R0(a.x[1]) * B_HAS_R1(b.x[0]) + A_HAS_R1(a.x[5]) * B_HAS_R0(b.x[1]) + A_HAS_R0(a.x[9])  * B_HAS_R0(b.x[2]) + A_HAS_T(a.x[13])  * B_HAS_P0(b.x[3]); \
	c->x[2] = A_HAS_R0(a.x[2]) * B_HAS_R1(b.x[0]) + A_HAS_R0(a.x[6]) * B_HAS_R0(b.x[1]) + A_HAS_R1(a.x[10]) * B_HAS_R0(b.x[2]) + A_HAS_T(a.x[14])  * B_HAS_P0(b.x[3]); \
	c->x[3] = A_HAS_P0(a.x[3]) * B_HAS_R1(b.x[0]) + A_HAS_P0(a.x[7]) * B_HAS_R0(b.x[1]) + A_HAS_P0(a.x[11]) * B_HAS_R0(b.x[2]) + A_HAS_P1(a.x[15]) * B_HAS_P0(b.x[3]); \
\
	c->x[4] = A_HAS_R1(a.x[0]) * B_HAS_R0(b.x[4]) + A_HAS_R0(a.x[4]) * B_HAS_R1(b.x[5]) + A_HAS_R0(a.x[8])  * B_HAS_R0(b.x[6]) + A_HAS_T(a.x[12])  * B_HAS_P0(b.x[7]); \
	c->x[5] = A_HAS_R0(a.x[1]) * B_HAS_R0(b.x[4]) + A_HAS_R1(a.x[5]) * B_HAS_R1(b.x[5]) + A_HAS_R0(a.x[9])  * B_HAS_R0(b.x[6]) + A_HAS_T(a.x[13])  * B_HAS_P0(b.x[7]); \
	c->x[6] = A_HAS_R0(a.x[2]) * B_HAS_R0(b.x[4]) + A_HAS_R0(a.x[6]) * B_HAS_R1(b.x[5]) + A_HAS_R1(a.x[10]) * B_HAS_R0(b.x[6]) + A_HAS_T(a.x[14])  * B_HAS_P0(b.x[7]); \
	c->x[7] = A_HAS_P0(a.x[3]) * B_HAS_R0(b.x[4]) + A_HAS_P0(a.x[7]) * B_HAS_R1(b.x[5]) + A_HAS_P0(a.x[11]) * B_HAS_R0(b.x[6]) + A_HAS_P1(a.x[15]) * B_HAS_P0(b.x[7]); \
\
	c->x[8]  = A_HAS_R1(a.x[0]) * B_HAS_R0(b.x[8]) + A_HAS_R0(a.x[4]) * B_HAS_R0(b.x[9]) + A_HAS_R0(a.x[8])  * B_HAS_R1(b.x[10]) + A_HAS_T(a.x[12])  * B_HAS_P0(b.x[11]); \
	c->x[9]  = A_HAS_R0(a.x[1]) * B_HAS_R0(b.x[8]) + A_HAS_R1(a.x[5]) * B_HAS_R0(b.x[9]) + A_HAS_R0(a.x[9])  * B_HAS_R1(b.x[10]) + A_HAS_T(a.x[13])  * B_HAS_P0(b.x[11]); \
	c->x[10] = A_HAS_R0(a.x[2]) * B_HAS_R0(b.x[8]) + A_HAS_R0(a.x[6]) * B_HAS_R0(b.x[9]) + A_HAS_R1(a.x[10]) * B_HAS_R1(b.x[10]) + A_HAS_T(a.x[14])  * B_HAS_P0(b.x[11]); \
	c->x[11] = A_HAS_P0(a.x[3]) * B_HAS_R0(b.x[8]) + A_HAS_P0(a.x[7]) * B_HAS_R0(b.x[9]) + A_HAS_P0(a.x[11]) * B_HAS_R1(b.x[10]) + A_HAS_P1(a.x[15]) * B_HAS_P0(b.x[11]); \
\
	c->x[12] = A_HAS_R1(a.x[0]) * B_HAS_T(b.x[12]) + A_HAS_R0(a.x[4]) * B_HAS_T(b.x[13]) + A_HAS_R0(a.x[8])  * B_HAS_T(b.x[14]) + A_HAS_T(a.x[12])  * B_HAS_P1(b.x[15]); \
	c->x[13] = A_HAS_R0(a.x[1]) * B_HAS_T(b.x[12]) + A_HAS_R1(a.x[5]) * B_HAS_T(b.x[13]) + A_HAS_R0(a.x[9])  * B_HAS_T(b.x[14]) + A_HAS_T(a.x[13])  * B_HAS_P1(b.x[15]); \
	c->x[14] = A_HAS_R0(a.x[2]) * B_HAS_T(b.x[12]) + A_HAS_R0(a.x[6]) * B_HAS_T(b.x[13]) + A_HAS_R1(a.x[10]) * B_HAS_T(b.x[14]) + A_HAS_T(a.x[14])  * B_HAS_P1(b.x[15]); \
	c->x[15] = A_HAS_P0(a.x[3]) * B_HAS_T(b.x[12]) + A_HAS_P0(a.x[7]) * B_HAS_T(b.x[13]) + A_HAS_P0(a.x[11]) * B_HAS_T(b.x[14]) + A_HAS_P1(a.x[15]) * B_HAS_P1(b.x[15]); 

void Matrix4::MultMatrix4Expanded(const Matrix4& a, const Matrix4& b, Matrix4* c)
{
	// This does not support the target being one of the sources.
	GLASSERT(c != &a && c != &b && &a != &b);
	float* dst = c->x;

	// The counters are rows and columns of 'c'
	for (int j = 0; j < 4; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			// for c:
			//      j increments the row
			//      i increments the column
			*dst++ = a.x[i + 0] * b.x[j * 4 + 0]
				+ a.x[i + 4] * b.x[j * 4 + 1]
				+ a.x[i + 8] * b.x[j * 4 + 2]
				+ a.x[i + 12] * b.x[j * 4 + 3];
		}
	}
}


void grinliz::MultMatrix4(const Matrix4& a, const Matrix4& b, Matrix4* c)
{
	GLASSERT(c != &a && c != &b && &a != &b);

	int aType = a.GetType();
	int bType = b.GetType();

	if (aType == Matrix4::IDENTITY_TYPE) {
		*c = b;
	}
	else if (bType == Matrix4::IDENTITY_TYPE) {
		*c = a;
	}
	else if (aType == Matrix4::T_TERM && b.type == Matrix4::T_TERM) {
		// Both translation
#undef A_HAS_T
#undef A_HAS_R0
#undef A_HAS_R1
#undef A_HAS_P0
#undef A_HAS_P1
#undef B_HAS_T
#undef B_HAS_R0
#undef B_HAS_R1
#undef B_HAS_P0
#undef B_HAS_P1

#define A_HAS_T(x)  x
#define A_HAS_R0(x) 0
#define A_HAS_R1(x) 1
#define A_HAS_P0(x) 0
#define A_HAS_P1(x) 1
#define B_HAS_T(x)  x
#define B_HAS_R0(x) 0
#define B_HAS_R1(x) 1
#define B_HAS_P0(x) 0
#define B_HAS_P1(x) 1
			MAT_MULT_4;
			c->type = Matrix4::UNKNOWN_TYPE;
	}
	else if ( ((aType & Matrix4::P_TERM) == 0) && ((bType & Matrix4::P_TERM) == 0)) {
		// Both NOT perspective
#undef A_HAS_T
#undef A_HAS_R0
#undef A_HAS_R1
#undef A_HAS_P0
#undef A_HAS_P1
#undef B_HAS_T
#undef B_HAS_R0
#undef B_HAS_R1
#undef B_HAS_P0
#undef B_HAS_P1

#define A_HAS_T(x)  x
#define A_HAS_R0(x) x
#define A_HAS_R1(x) x
#define A_HAS_P0(x) 0
#define A_HAS_P1(x) 1
#define B_HAS_T(x)  x
#define B_HAS_R0(x) x
#define B_HAS_R1(x) x
#define B_HAS_P0(x) 0
#define B_HAS_P1(x) 1
			MAT_MULT_4;
			c->type = Matrix4::UNKNOWN_TYPE;
	}
	else {
#undef A_HAS_T
#undef A_HAS_R0
#undef A_HAS_R1
#undef A_HAS_P0
#undef A_HAS_P1
#undef B_HAS_T
#undef B_HAS_R0
#undef B_HAS_R1
#undef B_HAS_P0
#undef B_HAS_P1

#define A_HAS_T(x)  x
#define A_HAS_R0(x) x
#define A_HAS_R1(x) x
#define A_HAS_P0(x) x
#define A_HAS_P1(x) x
#define B_HAS_T(x)  x
#define B_HAS_R0(x) x
#define B_HAS_R1(x) x
#define B_HAS_P0(x) x
#define B_HAS_P1(x) x
			MAT_MULT_4;
			c->type = Matrix4::UNKNOWN_TYPE;
	}

#ifdef MAT_DEBUG_DEEP
	c->GetType();	// double check

	Matrix4 altC;
	Matrix4::MultMatrix4Expanded(a, b, &altC);
	GLASSERT(Equal(altC, *c));
#endif
}

void Matrix4::ConcatTranslation(const Vector3F& t)
{
	Matrix4 a = *this;
	Matrix4 b;
	b.SetTranslation(t);
	*this = a * b;
}


void Matrix4::SetXRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = 1.0f;
	x[1] = 0.0f;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = cosTheta;
	x[6] = sinTheta;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = -sinTheta;
	x[10] = cosTheta;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;

	type = Matrix4::UNKNOWN_TYPE;
}


void Matrix4::SetYRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = 0.0f;
	x[2] = -sinTheta;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = 1.0f;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = sinTheta;
	x[9] = 0;
	x[10] = cosTheta;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;

	type = Matrix4::UNKNOWN_TYPE;
}

void Matrix4::SetZRotation( float theta )
{
	float cosTheta, sinTheta;
	SinCosDegree( theta, &sinTheta, &cosTheta );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = sinTheta;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = -sinTheta;
	x[5] = cosTheta;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = 0.0f;
	x[10] = 1.0f;

	// COLUMN 4
	x[12] = 0.0f;
	x[13] = 0.0f;
	x[14] = 0.0f;

	type = Matrix4::UNKNOWN_TYPE;
}


float Matrix4::CalcRotationAroundAxis( int axis ) const
{
	float theta = 0.0f;

	switch ( axis ) {
		case 0:		theta = atan2f( x[6], x[10] );		break;
		case 1:		theta = atan2f( x[8], x[0] );		break;
		case 2:		theta = atan2f( x[1], x[0] );		break;

		default:
			GLASSERT( 0 );
	}
	theta *= RAD_TO_DEG;
	if ( theta < 0.0f ) theta += 360.0f;

	return theta ;
}


void Matrix4::SetAxisAngle( const Vector3F& axis, float angle )
{
	angle *= DEG_TO_RAD;
	float c = (float) cos( angle );
	float s = (float) sin( angle );
	float t = 1.0f - c;

	x[0]  = t*axis.x*axis.x + c;
	x[1]  = t*axis.x*axis.y - s*axis.z;
	x[2]  = t*axis.x*axis.z + s*axis.y;
	x[3]  = 0.0f;

	x[4]  = t*axis.x*axis.y + s*axis.z;
	x[5]  = t*axis.y*axis.y + c;
	x[6]  = t*axis.y*axis.z - s*axis.x;
	x[7]  = 0.0f;

	x[8]  = t*axis.x*axis.z - s*axis.y;
	x[9]  = t*axis.y*axis.z + s*axis.x;
	x[10] = t*axis.z*axis.z + c;
	x[11] = 0.0f;

	x[12] = x[13] = x[14] = 0.0f;
	x[15] = 1.0f;

	type = Matrix4::UNKNOWN_TYPE;
}


void Matrix4::SetFrustum( float left, float right, float bottom, float top, float near, float far )
{
	SetIdentity();

	if ( near <= 0.0 || far <= 0.0 || near == far || left == right || top == bottom)
	{
		GLASSERT( 0 );
	}
	else {
		float x = (2.0f*near) / (right-left);
		float y = (2.0f*near) / (top-bottom);
		float a = (right+left) / (right-left);
		float b = (top+bottom) / (top-bottom);
		float c = -(far+near) / ( far-near);
		float d = -(2.0f*far*near) / (far-near);		

		Set( 0, 0, x );		Set( 0, 1, 0 );		Set( 0, 2, a );		Set( 0, 3, 0 );
		Set( 1, 0, 0 );		Set( 1, 1, y );		Set( 1, 2, b );		Set( 1, 3, 0 );
		Set( 2, 0, 0 );		Set( 2, 1, 0 );		Set( 2, 2, c );		Set( 2, 3, d );
		Set( 3, 0, 0 );		Set( 3, 1, 0 );		Set( 3, 2, -1.0f );	Set( 3, 3, 0 );
	}
	type = Matrix4::UNKNOWN_TYPE;
}


void Matrix4::SetOrtho( float left, float right, float bottom, float top, float zNear, float zFar )
{
	SetIdentity();
	if ( ( right != left ) && ( zFar != zNear ) && ( top != bottom ) ) { 
		Set( 0, 0, 2.0f / (right-left ) );	
		Set( 0, 1, 0 );						
		Set( 0, 2, 0 );		
		Set( 0, 3, -(right+left) / (right-left) );

		Set( 1, 0, 0 );						
		Set( 1, 1, 2.0f / (top-bottom) );	
		Set( 1, 2, 0 );		
		Set( 1, 3, -(top+bottom) / (top-bottom) );
		
		Set( 2, 0, 0 );						
		Set( 2, 1, 0 );						
		Set( 2, 2, -2.0f / (zFar-zNear) );	
		Set( 2, 3, -(zFar+zNear) / (zFar-zNear) );

		Set( 3, 0, 0 );
		Set( 3, 1, 0 );
		Set( 3, 2, 0 );
		Set( 3, 3, 1 );
	}
	else {
		GLASSERT( 0 );
	}
	type = Matrix4::UNKNOWN_TYPE;
}


bool Matrix4::IsRotation() const
{
	// Check the rows and columns:

	for( int i=0; i<4; ++i )
	{
		float row = 0.0f;
		float col = 0.0f;
		for( int j=0; j<4; ++j )
		{
			row += x[i+j*4]*x[i+j*4];
			col += x[i*4+j]*x[i*4+j];
		}
		if ( !Equal( row, 1.0f, 0.0001f ) )
			return false;
		if ( !Equal( col, 1.0f, 0.0001f ) )
			return false;
	}
	// Should also really check orthogonality.
	return true;
}


void Matrix4::Transpose( Matrix4* transpose ) const
{
	GLASSERT( transpose != this );
	GLASSERT(transpose);
	if (!transpose) return;

	for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
			transpose->x[INDEX(c,r)] = x[INDEX(r,c)];
        }
    }
}

void Matrix4::Transpose()
{
	Matrix4 m = *this;
	for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
			this->x[INDEX(c,r)] = m.x[INDEX(r,c)];
        }
    }
	type = Matrix4::UNKNOWN_TYPE;
}


float Matrix4::Determinant() const
{
	//	11 12 13 14
	//	21 22 23 24
	//	31 32 33 34
	//	41 42 43 44

	// Old school. Derived from Graphics Gems 1
	return x[M11] * Determinant3x3(	x[M22], x[M23], x[M24], 
									x[M32], x[M33], x[M34], 
									x[M42], x[M43], x[M44] ) -
		   x[M12] * Determinant3x3(	x[M21], x[M23], x[M24], 
									x[M31], x[M33], x[M34], 
									x[M41], x[M43], x[M44] ) +
		   x[M13] * Determinant3x3(	x[M21], x[M22], x[M24],
									x[M31], x[M32], x[M34], 
									x[M41], x[M42], x[M44] ) -
		   x[M14] * Determinant3x3(	x[M21], x[M22], x[M23], 
									x[M31], x[M32], x[M33], 
									x[M41], x[M42], x[M43] );
}


void Matrix4::Adjoint( Matrix4* adjoint ) const
{
	Matrix4 cofactor;
	Cofactor( &cofactor );
	cofactor.Transpose( adjoint );
	adjoint->type = UNKNOWN_TYPE;
}


void Matrix4::Invert( Matrix4* inverse ) const
{
	GetType();
	if (type == R_TERM) {
		InvertRotationMat(inverse);
	}
	else {
		// The inverse is the adjoint / determinant of adjoint
		Adjoint(inverse);
		float d = Determinant();
		inverse->ApplyScalar(1.0f / d);
		inverse->type = UNKNOWN_TYPE;
	}
}


void Matrix4::InvertRotationMat( Matrix4* inverse ) const
{
	// http://graphics.stanford.edu/courses/cs248-98-fall/Final/q4.html
	*inverse = *this;

	// Transpose the rotation terms.
	Swap( &inverse->x[M12], &inverse->x[M21] );
	Swap( &inverse->x[M13], &inverse->x[M31] );
	Swap( &inverse->x[M23], &inverse->x[M32] );

	const Vector3F* u = (const Vector3F*)(&x[0]);
	const Vector3F* v = (const Vector3F*)(&x[4]);
	const Vector3F* w = (const Vector3F*)(&x[8]);
	const Vector3F* t = (const Vector3F*)(&x[12]);

	inverse->x[M14] = -DotProduct( *u, *t );
	inverse->x[M24] = -DotProduct( *v, *t );
	inverse->x[M34] = -DotProduct( *w, *t );
	inverse->type = UNKNOWN_TYPE;
}


void Matrix4::Cofactor( Matrix4* cofactor ) const
{
    int i = 1;

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float det = SubDeterminant(r, c);
            cofactor->x[INDEX(r,c)] = i * det;	// i flip flops between 1 and -1
            i = -i;
        }
        i = -i;
    }
	cofactor->type = UNKNOWN_TYPE;
}


float Matrix4::SubDeterminant(int excludeRow, int excludeCol) const 
{
    // Compute non-excluded row and column indices
    int row[3];
    int col[3];

    for (int i = 0; i < 3; ++i) {
        row[i] = i;
        col[i] = i;

        if (i >= excludeRow) {
            ++row[i];
        }
        if (i >= excludeCol) {
            ++col[i];
        }
    }

    // Compute the first row of cofactors 
    float cofactor00 = 
      x[INDEX(row[1],col[1])] * x[INDEX(row[2],col[2])] -
      x[INDEX(row[1],col[2])] * x[INDEX(row[2],col[1])];

    float cofactor10 = 
      x[INDEX(row[1],col[2])] * x[INDEX(row[2],col[0])] -
      x[INDEX(row[1],col[0])] * x[INDEX(row[2],col[2])];

    float cofactor20 = 
      x[INDEX(row[1],col[0])] * x[INDEX(row[2],col[1])] -
      x[INDEX(row[1],col[1])] * x[INDEX(row[2],col[0])];

    // Product of the first row and the cofactors along the first row
    return      x[INDEX(row[0],col[0])] * cofactor00 +
				x[INDEX(row[0],col[1])] * cofactor10 +
				x[INDEX(row[0],col[2])] * cofactor20;
}

//
// http://www.opengl.org/wiki/GluLookAt_code
//
void grinliz::LookAt( bool cameraFlipBug,
					  const Vector3F& eye, const Vector3F& center, const Vector3F& up,
					  Matrix4* rot, Matrix4* trans, Matrix4* view )
{
	Matrix4 _rot, _trans, _view;
	if ( !rot )
		rot = &_rot;
	if ( !trans )
		trans = &_trans;
	if ( !view )
		view = &_view;

	Vector3F forward = center - eye;
	forward.Normalize();

    /* Side = forward x up */
	Vector3F side;
	CrossProduct(forward, up, &side);
    side.Normalize();

    /* Recompute up as: up = side x forward */
	Vector3F up2;
    CrossProduct(side, forward, &up2);

	if ( cameraFlipBug ) {
		// GAH FIXME this should all be SetRow. 
		// Something has gone screwy in the camera code. Need
		// to root out the issue.
		rot->SetCol( 0, side.x, side.y, side.z, 0.0f );
		rot->SetCol( 1, up2.x, up2.y, up2.z, 0.0f );
		rot->SetCol( 2, -forward.x, -forward.y, -forward.z, 0 );
		rot->SetCol( 3, 0, 0, 0, 1 );
	}
	else {
		rot->SetRow( 0, side.x, side.y, side.z, 0.0f );
		rot->SetRow( 1, up2.x, up2.y, up2.z, 0.0f );
		rot->SetRow( 2, -forward.x, -forward.y, -forward.z, 0 );
		rot->SetRow( 3, 0, 0, 0, 1 );
	}
	trans->SetIdentity();
	trans->SetTranslation( -eye.x, -eye.y, -eye.z );

	*view = (*rot) * (*trans);
}


void grinliz::MultMatrix4( const Matrix4& m, const Rectangle3F& in, Rectangle3F* out )
{
	// http://www.gamedev.net/topic/349370-transform-aabb-from-local-to-world-space-for-frustum-culling/

	out->Set( m.X(12), m.X(13), m.X(14),
		      m.X(12), m.X(13), m.X(14) );

	for( int i=0; i<3; ++i ) {
		for( int j=0; j<3; ++j ) {
			float av = m.m(i,j) * in.min.X(j);
			float bv = m.m(i,j) * in.max.X(j);
			if (av < bv)
			{
				out->min.X(i) += av;
				out->max.X(i) += bv;
			} else {
				out->min.X(i) += bv;
				out->max.X(i) += av;
			}
		}
	}
}


void Matrix2::SetRotation( float degrees )
{
	float rad = ToRadian( degrees );
	float co = cosf( rad );
	float si = sinf( rad );
	a = co;
	b = -si;
	c = si;
	d = co;
}


void Matrix4::Test()
{
	const Matrix4 identity;

	{
		// Simple transformation tests.
		Matrix4 t, r;
		Vector3F v = { 1, 0, 0 };
		Vector3F vt = { 0, 1, 1 };
		t.SetTranslation(vt);
		r.SetZRotation(45.0f);

		Vector3F vPrime = t * v;
		Vector3F answer = { 1, 1, 1 };
		GLASSERT(Equal(vPrime, answer));

		vPrime = r * v;
		answer.Set(0.70711f, 0.70711f, 0);
		GLASSERT(Equal(vPrime, answer));

		Matrix4 m0, m1, m2;
		m0 = r * t;
		MultMatrix4(r, t, &m1);

		m2 = r;

		m2.ConcatTranslation(vt);

		GLASSERT(Equal(m0, m1));
		GLASSERT(Equal(m0, m2));

		vPrime = m0 * v;
		answer.Set(0, 1.4142, 1);
		GLASSERT(Equal(vPrime, answer));

		Matrix4 m0Inv;
		m0.Invert(&m0Inv);
		Matrix4 ident0 = m0 * m0Inv;
		GLASSERT(Equal(ident0, identity));
		(void)ident0;

		Matrix4 rInv;
		r.InvertRotationMat(&rInv);
		Matrix4 ident1 = r * rInv;
		GLASSERT(Equal(ident1, identity));
		(void)ident1;
	}

	GLOUTPUT(("Matrix test complete.\n"));

}

