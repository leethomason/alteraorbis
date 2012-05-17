#ifndef CAMERA_COMPONENT_INCLUDED
#define CAMERA_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class Camera;

// Also needs other modes.
class CameraComponent : public Component
{
public:
	CameraComponent( Camera* _camera ) 
		:	camera( _camera ),
			autoDelete( false ),
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

	void SetAutoDelete( bool ad )				{ autoDelete = ad; }
	void SetPanTo( grinliz::Vector3F& dest, float speed = 40.0f );

private:
	Camera* camera;
	bool autoDelete;

	enum {
		DONE,	// will delete
		PAN
	};
	int mode;
	grinliz::Vector3F dest;
	float speed;
};

#endif // CAMERA_COMPONENT_INCLUDED