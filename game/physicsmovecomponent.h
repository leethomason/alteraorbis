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

	void Set( const grinliz::Vector3F& _velocity )		{ velocity = _velocity; }

private:
	grinliz::Vector3F velocity;
};


#endif // PHYSICS_MOVE_COMPONENT_INCLUDED
