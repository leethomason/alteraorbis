#ifndef PHYSICS_MOVE_COMPONENT_INCLUDED
#define PHYSICS_MOVE_COMPONENT_INCLUDED

#include "gamemovecomponent.h"

class WorldMap;

class PhysicsMoveComponent : public GameMoveComponent
{
private:
	typedef GameMoveComponent super;
public:
	PhysicsMoveComponent( WorldMap* _map );
	virtual ~PhysicsMoveComponent() {}

	virtual const char* Name() const { return "PhysicsMoveComponent"; }
	virtual void DebugStr( grinliz::GLString* str );

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual int DoTick( U32 delta, U32 since );
	virtual bool IsMoving() const;
	virtual void CalcVelocity( grinliz::Vector3F* v )	{ *v = velocity; }

	void Set( const grinliz::Vector3F& _velocity, float _rotation )		{ velocity = _velocity; rotation = _rotation; }
	void Add( const grinliz::Vector3F& _velocity, float _rotation )		{ velocity += _velocity; rotation += _rotation; }

	void DeleteWhenDone( bool _deleteWhenDone )			{ deleteWhenDone = _deleteWhenDone; }

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	grinliz::Vector3F velocity;
	float rotation;
	bool deleteWhenDone;
};


#endif // PHYSICS_MOVE_COMPONENT_INCLUDED
