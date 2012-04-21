#ifndef XENOENGINE_COMPONENT_INCLUDED
#define XENOENGINE_COMPONENT_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

// The primary components:
class SpatialComponent;
class RenderComponent;
class MoveComponent;
class Chit;

class Component
{
public:
	virtual void OnAdd( Chit* chit )	{	parentChit = chit; }
	virtual void OnRemove()				{	parentChit = 0;    }

	virtual SpatialComponent*	ToSpatial()		{ return 0; }
	virtual MoveComponent*		ToMove()		{ return 0; }
	virtual RenderComponent*	ToRender()		{ return 0; }

	virtual bool NeedsTick()					{ return false; }

	// Tick is a regular call; update because of events/change.
	virtual void DoTick( U32 delta )			{}
	virtual void DoUpdate()						{}

protected:
	void RequestUpdate();
	Chit* parentChit;
};


// Abstract at the XenoEngine level.
class MoveComponent : public Component
{
public:
	virtual MoveComponent*		ToMove()		{ return this; }
};


#endif // XENOENGINE_COMPONENT_INCLUDED