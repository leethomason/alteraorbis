/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gpustatemanager.h"
#include "camera.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmatrix.h"
#include "serialize.h"

using namespace grinliz;

Camera::Camera() : 
	valid( false )
{
	posWC.Set( 0.f, 0.f, 0.f );
	TiltRotationToQuat( 45.0f, 0. );
}


void Camera::TiltRotationToQuat( float tilt, float yRotation )
{
	Matrix4 rotationY, rotationTilt;
	rotationY.SetYRotation( yRotation );
	rotationTilt.SetXRotation( tilt );

	// Done in world: we tilt it down, turn it around y, then move it.
	Matrix4 m = rotationY * rotationTilt;
	quat.FromRotationMatrix( m );
}


void Camera::CalcWorldXForm()
{
	if ( !valid ) {
		Matrix4 translation, rotation;

		translation.SetTranslation( posWC );

		quat.Normalize();
		quat.ToMatrix( &rotation );

		worldXForm = translation * rotation;
		CalcEyeDir();

		// Calc the View Matrix
		Matrix4 inv, zRot;
		worldXForm.Invert( &inv );
		viewMatrix = zRot*inv;

		valid = true;
	}
}


void Camera::CalcEyeDir()
{
	Vector4F dir[3] = {
		{ 0.0f, 0.0f, -1.0f, 0.0f },	// ray
		{ 0.0f, 1.0f, 0.0f, 0.0f },		// up (y axis)
		{ 1.0f, 0.0f, 0.0f, 0.0f },		// right (x axis)
	};

	for( int i=0; i<3; ++i ) {
		eyeDir[i] = worldXForm * dir[i];
		eyeDir3[i].x = eyeDir[i].x;
		eyeDir3[i].y = eyeDir[i].y;
		eyeDir3[i].z = eyeDir[i].z;
	}
}


void Camera::Orbit( float rotation )
{
	// This is kind of a tricky function. We want to rotate around
	// what the camera is looking at, not the origin of the camera.
	// It's a useful function, but a pain in the ass.

	EyeDir();			// bring cache current
	Vector3F pole;		// the pole is up; this is the base of the pole, essentially
	if ( IntersectRayPlane( posWC, eyeDir3[0], 1, 0, &pole ) == INTERSECT ) {

		Vector3F delta = posWC - pole;

		// Append the rotation
		Matrix4 rotMat, current, m;
		rotMat.SetYRotation( rotation );
		quat.ToMatrix( &current );
		m = rotMat * current;
		quat.FromRotationMatrix( m );

		// Move the origin to componensate
		posWC = pole + rotMat * delta;

#if 0
		Matrix4 r;
		r.SetYRotation( yRotation + rotation );

		float length = sqrtf( (pole.x-posWC.x)*(pole.x-posWC.x) + (pole.z-posWC.z)*(pole.z-posWC.z) );

		Vector3F vec = { 0.0f, 0.0f, length };
		Vector3F vecPrime = r * vec;
		vecPrime.y = posWC.y;

		posWC = pole + vecPrime;

		yRotation += rotation;
#endif
	}
	valid = false;
}
