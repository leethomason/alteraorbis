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
#include "../xarchive/glstreamer.h"

using namespace grinliz;

Camera::Camera() : 
	valid( false )
{
	posWC.Set( 0.f, 0.f, 0.f );
	TiltRotationToQuat( 45.0f, 0. );
}


void Camera::Load( const tinyxml2::XMLElement* element )
{
	const tinyxml2::XMLElement* camEle = element->FirstChildElement( "Camera" );
	GLASSERT( camEle );
	if ( camEle ) {
		XEArchive( 0, camEle, "posWC", posWC );
		XEArchive( 0, camEle, "quat",  quat );
	}
}


void Camera::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "Camera" );
	XEArchive( printer, 0, "posWC", posWC );
	XEArchive( printer, 0, "quat",  quat );
	printer->CloseElement();
}


void Camera::Serialize( XStream* xs )
{
	XarcOpen( xs, "Camera" );
	XARC_SER( xs, posWC );
	XARC_SER( xs, quat );
	XarcClose( xs );
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


void Camera::SetDir( const grinliz::Vector3F& _dir, const grinliz::Vector3F& up )
{
	Matrix4 m;
	Vector3F dir = _dir;
	dir.Normalize();

	static const Vector3F zero = { 0, 0, 0 };
	LookAt( true, zero, dir, up, &m, 0, 0 );
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
		worldXForm.Invert( &viewMatrix );

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
		eyeDir3[i].Set( eyeDir[i].x, eyeDir[i].y, eyeDir[i].z );
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
	}
	valid = false;
}
