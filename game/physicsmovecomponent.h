#ifndef PHYSICS_MOVE_COMPONENT_INCLUDED
#define PHYSICS_MOVE_COMPONENT_INCLUDED

#include "gamemovecomponent.h"

class WorldMap;

class PhysicsMoveComponent : public GameMoveComponent
{
public:
	PhysicsMoveComponent( WorldMap* _map );
	virtual ~PhysicsMoveComponent() {}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "PhysicsMoveComponent" ) ) return this;
		return GameMoveComponent::ToComponent( name );
	}
	virtual void DebugStr( grinliz::GLString* str );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual bool DoTick( U32 delta );
	virtual bool IsMoving() const;
	virtual void CalcVelocity( grinliz::Vector3F* v )	{ *v = velocity; }

	void Set( const grinliz::Vector3F& _velocity, float _rotation )		{ velocity = _velocity; rotation = _rotation; }
	void Add( const grinliz::Vector3F& _velocity, float _rotation )		{ velocity += _velocity; rotation += _rotation; }

	void DeleteWhenDone( bool _deleteWhenDone )			{ deleteWhenDone = _deleteWhenDone; }

private:
	grinliz::Vector3F velocity;
	float rotation;
	bool deleteWhenDone;
};


#endif // PHYSICS_MOVE_COMPONENT_INCLUDED
