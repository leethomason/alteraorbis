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

	virtual SpatialComponent*	ToSpatial()		{ return this; }

private:
	grinliz::Vector3F position;
};

#endif // SPACIAL_COMPONENT_INCLUDED