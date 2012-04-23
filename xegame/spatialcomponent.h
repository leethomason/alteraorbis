#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"

class SpatialComponent : public Component
{
public:
	SpatialComponent() {
		position.Zero();
	}

	const char* Name() const { return "SpatialComponent"; }

	virtual SpatialComponent*	ToSpatial()			{ return this; }

	void SetPosition( float x, float y, float z )	{ position.Set( x, y, z ); RequestUpdate(); }
	const grinliz::Vector3F& GetPosition()			{ return position; }

private:
	grinliz::Vector3F position;
};

#endif // SPACIAL_COMPONENT_INCLUDED