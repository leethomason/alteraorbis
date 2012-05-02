#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class SpatialComponent : public Component
{
public:
	SpatialComponent() {
		position.Zero();
		yRotation = 0;
	}

	virtual const char* Name() const { return "SpatialComponent"; }

	virtual SpatialComponent*	ToSpatial()			{ return this; }

	void SetPosition( float x, float y, float z );
	const grinliz::Vector3F& GetPosition() const	{ return position; }

	// yRot=0 is the +z axis
	void SetYRotation( float yDegrees )				{ yRotation = grinliz::NormalizeAngleDegrees( yDegrees ); }
	float GetYRotation() const						{ return yRotation; }

	grinliz::Vector3F GetHeading() const;

	grinliz::Vector2F GetPosition2D() const			{ grinliz::Vector2F v = { position.x, position.z }; return v; }
	grinliz::Vector2F GetHeading2D() const;

private:
	grinliz::Vector3F	position;
	float				yRotation;	// [0, 360)
};

#endif // SPACIAL_COMPONENT_INCLUDED