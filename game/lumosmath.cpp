#include "lumosmath.h"

using namespace grinliz;


float YRotation(const grinliz::Quaternion& rotation)
{
	Vector3F axis;
	float angle;
	rotation.ToAxisAngle( &axis, &angle );
	if (axis.y < 0) {
		axis = axis * -1.0f;
		angle = -angle;
	}

#ifdef DEBUG
	static const Vector3F UP = {0,1,0};
	GLASSERT( angle == 0 ||  DotProduct( UP, axis ) > 0.99f );	// else probably not what was intended.
#endif
	return NormalizeAngleDegrees( angle );
}
