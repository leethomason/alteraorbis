#ifndef CAMERA_COMPONENT_INCLUDED
#define CAMERA_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class Camera;

class CameraComponent : public Component
{
public:
	CameraComponent( Camera* _camera ) 
		:	camera( _camera ),
			mode( DONE ),
			speed( 0 )
	{
		dest.Zero();
	}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "CameraComponent" ) ) return this;
		return Component::ToComponent( name );
	}
	virtual void DebugStr( grinliz::GLString* str );
	virtual void DoTick( U32 delta );
	virtual bool NeedsTick()					{ return true; }

	void SetPanTo( grinliz::Vector3F& dest, float speed = 40.0f );

private:
	Camera* camera;
	enum {
		DONE,	// will delete
		PAN
	};
	int mode;
	grinliz::Vector3F dest;
	float speed;
};

#endif // CAMERA_COMPONENT_INCLUDED