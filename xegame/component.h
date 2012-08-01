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

	// fixme: switch to a request/release model?
	virtual bool NeedsTick()					{ return false; }

	Chit* ParentChit() { return parentChit; }

	// Tick is a regular call; update because of events/change.
	virtual void DoTick( U32 delta )			{}
	virtual void DoSlowTick()					{}
	virtual void DoUpdate()						{}
	virtual void DebugStr( grinliz::GLString* str )		{}
	// 'chit' and/or 'event' can be null, depending on event
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* event )	{}

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