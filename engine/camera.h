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

#ifndef UFOATTACK_CAMERA_INCLUDED
#define UFOATTACK_CAMERA_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"

class XStream;

/*	By default, the camera is looking down the -z axis with y up.
	View rotation is handled by this class to account for device
	turning.
*/
class Camera
{
public:
	Camera();
	~Camera()	{}

	// Convert from the old system:
	void TiltRotationToQuat( float tilt, float yRotation );
	void SetDir( const grinliz::Vector3F& dir, const grinliz::Vector3F& up );

	// Move the camera around a center pole. The idea of rotation from the users point of view.
	void Orbit( float delta );

	// Position in world coordinates.
	const grinliz::Vector3F& PosWC() const			{
		return posWC;
	}
	void SetPosWC(const grinliz::Vector3F& value)	{
		posWC = value;
		valid = false;
	}
	void SetPosWC(float x, float y, float z)		{
		posWC.Set(x, y, z);
		valid = false;
	}
	void DeltaPosWC(float x, float y, float z)	{
		posWC.x += x; posWC.y += y; posWC.z += z;
		valid = false;
	}

	const grinliz::Matrix4& ViewMatrix()			{ if ( !valid ) CalcWorldXForm();
													  return viewMatrix;
													}
	const grinliz::Quaternion& Quat() const			{ return quat; }
	void SetQuat(const grinliz::Quaternion& q)		{
		quat = q;
		valid = false;
	}
								
	enum {
		NORMAL,
		UP,
		RIGHT
	};
	const grinliz::Vector4F* EyeDir()				{ if ( !valid ) CalcWorldXForm();
													  return eyeDir; }	
	// normal(0), up(1), right(2)
	const grinliz::Vector3F* EyeDir3()				{ if ( !valid ) CalcWorldXForm();
													  return eyeDir3; }

	void Serialize( XStream* xs );

private:
	// Position of the camera in the world - no view rotation, no inverse.
	void CalcWorldXForm();
	void CalcEyeDir();

	grinliz::Quaternion quat;
	grinliz::Vector3F posWC;

	bool valid;
	grinliz::Vector4F eyeDir[3];
	grinliz::Vector3F eyeDir3[3];
	grinliz::Matrix4 worldXForm, viewMatrix;
};


#endif // UFOATTACK_CAMERA_INCLUDED

