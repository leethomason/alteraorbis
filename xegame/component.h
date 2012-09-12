#ifndef XENOENGINE_COMPONENT_INCLUDED
#define XENOENGINE_COMPONENT_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glstringutil.h"

// The primary components:
class SpatialComponent;
class RenderComponent;
class MoveComponent;
class Chit;
class ChitBag;
class ChitEvent;
class ChitMsg;

class Component
{
public:
	Component() : parentChit( 0 ), id( idPool++ )	{}
	virtual ~Component()			{}

	int ID() const { return id; }

	virtual void OnAdd( Chit* chit )	{	parentChit = chit; }
	virtual void OnRemove()				{	parentChit = 0;    }

	virtual Component* ToComponent( const char* name ) = 0 {
		if ( grinliz::StrEqual( name, "Component" ) ) return this;
		return 0;
	}

	// Utility
	Chit* ParentChit() { return parentChit; }
	Chit* GetChit( int id );

	// Tick is a regular call; update because of events/change.
	virtual bool DoTick( U32 delta )			{ return false; }	// returns whether future tick needed
	virtual bool DoSlowTick()					{ return false; }	// returns whether future tick needed
	virtual void DoUpdate()						{}
	virtual void DebugStr( grinliz::GLString* str )		{}

	virtual void OnChitEvent( const ChitEvent& event )			{}
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg )	{}

protected:
	void RequestUpdate();

	float Travel( float rate, U32 time ) const {
		return rate * ((float)time) * 0.001f;
	}

	ChitBag* GetChitBag();
	Chit* parentChit;
	int id;

	static int idPool;
};


// Abstract at the XenoEngine level.
class MoveComponent : public Component
{
public:
	virtual MoveComponent*		ToMove()		{ return this; }
	virtual bool IsMoving() const				{ return false; }

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "MoveComponent" ) ) return this;
		return Component::ToComponent( name );
	}
};


#endif // XENOENGINE_COMPONENT_INCLUDED