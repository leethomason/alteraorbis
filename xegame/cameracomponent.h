#ifndef CAMERA_COMPONENT_INCLUDED
#define CAMERA_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class CameraComponent : public Component
{
public:
	CameraComponent() {
		mode = DONE;
		dest.Zero();
	}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "CameraComponent" ) ) return this;
		return Component::ToComponent( name );
	}
	virtual void DebugStr( grinliz::GLString* str );
	virtual void DoTick( U32 delta )			{}

	void SetPanTo( grinliz::Vector3F& dest, float speed = 3.0f );

private:
	enum {
		DONE,	// will delete
		PAN
	};
	int mode;
	grinliz::Vector3F dest;
	float speed;
};

#endif // CAMERA_COMPONENT_INCLUDED