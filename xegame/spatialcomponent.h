#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "chit.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class RelativeSpatialComponent;

class SpatialComponent : public Component
{
public:
	//  track: should this be tracked in the ChitBag's spatial hash?
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
	virtual RelativeSpatialComponent* ToRelative()	{ return 0; }
	virtual void DebugStr( grinliz::GLString* str );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	// Position and rotation are absolute (post-transform)
	void SetPosition( float x, float y, float z )	{ SetPosYRot( x, y, z, yRotation ); }
	void SetPosition( const grinliz::Vector3F& v )	{ SetPosYRot( v, yRotation ); }	
	const grinliz::Vector3F& GetPosition() const	{ return position; }

	// yRot=0 is the +z axis
	void SetYRotation( float yDegrees )				{ SetPosYRot( position, yDegrees ); }
	float GetYRotation() const						{ return yRotation; }

	void SetPosYRot( float x, float y, float z, float yRot );
	void SetPosYRot( const grinliz::Vector3F& v, float yRot ) { SetPosYRot( v.x, v.y, v.z, yRot ); }

	grinliz::Vector3F GetHeading() const;

	grinliz::Vector2F GetPosition2D() const			{ grinliz::Vector2F v = { position.x, position.z }; return v; }
	grinliz::Vector2F GetHeading2D() const;

protected:
	grinliz::Vector3F	position;
	float				yRotation;	// [0, 360)
	bool				track;
};


class RelativeSpatialComponent : public SpatialComponent
{
public:
	RelativeSpatialComponent( bool track ) : SpatialComponent( track ) {
		relativePosition.Zero();
		relativeYRotation = 0;
	}
	virtual ~RelativeSpatialComponent()	{}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "RelativeSpatialComponent" ) ) return this;
		return SpatialComponent::ToComponent( name );
	}
	virtual RelativeSpatialComponent* ToRelative()	{ return this; }

	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, int id );

	void SetRelativePosYRot( float x, float y, float z, float rot );
	void SetRelativePosYRot( const grinliz::Vector3F& v, float rot )	{ SetRelativePosYRot( v.x, v.y, v.z, rot ); }

private:
	grinliz::Vector3F relativePosition;
	float relativeYRotation;
};

#endif // SPACIAL_COMPONENT_INCLUDED
