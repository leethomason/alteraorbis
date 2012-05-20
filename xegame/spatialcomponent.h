#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class SpatialComponent : public Component
{
public:
	SpatialComponent( bool _track ) {
		position.Zero();
		yRotation = 0;
		track = _track;
	}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "SpatialComponent" ) ) return this;
		return Component::ToComponent( name );
	}
	virtual SpatialComponent*	ToSpatial()			{ return this; }
	virtual void DebugStr( grinliz::GLString* str );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

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
	bool				track;
};

#endif // SPACIAL_COMPONENT_INCLUDED