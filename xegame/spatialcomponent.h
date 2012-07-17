#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "chit.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"

#include "../engine/enginelimits.h"

class RelativeSpatialComponent;

class SpatialComponent : public Component
{
public:
	//  track: should this be tracked in the ChitBag's spatial hash?
	SpatialComponent( bool _track ) {
		position.Zero();

		static const grinliz::Vector3F UP = { 0, 1, 0 };
		rotation.FromAxisAngle( UP, 0 );
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
	void SetPosition( float x, float y, float z )	{ grinliz::Vector3F v = { x, y, z }; SetPosition( v ); }
	void SetPosition( const grinliz::Vector3F& v )	{ SetPosRot( v, rotation ); }
	const grinliz::Vector3F& GetPosition() const	{ return position; }

	// yRot=0 is the +z axis
	void SetYRotation( float yDegrees )				{ SetPosYRot( position, yDegrees ); }
	float GetYRotation() const;
	const grinliz::Quaternion& GetRotation() const	{ return rotation; }

	void SetPosYRot( float x, float y, float z, float yRot );
	void SetPosYRot( const grinliz::Vector3F& v, float yRot ) { SetPosYRot( v.x, v.y, v.z, yRot ); }
	void SetPosRot( const grinliz::Vector3F& v, const grinliz::Quaternion& quat );

	grinliz::Vector3F GetHeading() const;

	grinliz::Vector2F GetPosition2D() const			{ grinliz::Vector2F v = { position.x, position.z }; return v; }
	grinliz::Vector2F GetHeading2D() const;

	//void AddChild( Chit* chit )			{ children.Add( chit ); }
	//void RemoveChild( Chit* chit )		{ children.Remove( chit ); }

protected:
	grinliz::Vector3F	position;
	grinliz::Quaternion	rotation;
	bool				track;
	//SafeChitList		children;
};


#if 0
// Currently attaches to:
//		- a metaData point on another chit.
//		- a relative offset fromm another chit (currently commented out)
class RelativeSpatialComponent : public SpatialComponent
{
public:
	RelativeSpatialComponent( bool track ) : SpatialComponent( track ) {
	}
	virtual ~RelativeSpatialComponent()	{}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "RelativeSpatialComponent" ) ) return this;
		return SpatialComponent::ToComponent( name );
	}
	virtual RelativeSpatialComponent* ToRelative()	{ return this; }

	virtual void DebugStr( grinliz::GLString* str );

	// Track to metadata on another chit.
	void SetMetaDataToTrack( const char* metaName )	{ metaData = metaName; }

private:
	grinliz::CStr< EL_RES_NAME_LEN > metaData;
};
#endif

#endif // SPACIAL_COMPONENT_INCLUDED
