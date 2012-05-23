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

class Component
{
public:
	Component() : parentChit( 0 ), id( idPool++ )	{}
	virtual ~Component()			{}

	int ID() const { return id; }

	virtual void OnAdd( Chit* chit )	{	parentChit = chit; }
	virtual void OnRemove()				{	parentChit = 0;    }

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "Component" ) ) return this;
		return 0;
	}
	virtual SpatialComponent*	ToSpatial()		{ return 0; }
	virtual MoveComponent*		ToMove()		{ return 0; }
	virtual RenderComponent*	ToRender()		{ return 0; }

	// fixme: switch to a request/release model?
	virtual bool NeedsTick()					{ return false; }

	Chit* ParentChit() { return parentChit; }

	// Tick is a regular call; update because of events/change.
	virtual void DoTick( U32 delta )			{}
	virtual void DoUpdate()						{}
	virtual void DebugStr( grinliz::GLString* str )		{}
	virtual void OnChitMsg( Chit* chit, int id )	{}

protected:
	void RequestUpdate();
	void SendMessage( int id );

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
};


#endif // XENOENGINE_COMPONENT_INCLUDED