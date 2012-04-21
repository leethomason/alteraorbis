#ifndef XENOENGINE_CHIT_INCLUDED
#define XENOENGINE_CHIT_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

#include "../SimpleLib/SimpleLib.h"

class Component;
class SpatialComponent;
class RenderComponent;
class MoveComponent;

class ChitBag;

/* General purpose GameObject.
   A class to hold Components.
	
*/
class Chit
{
public:
	Simple::CChain<Chit> m_Chain;

	Chit();
	~Chit();

	void OnAdd( ChitBag* c )	{ GLASSERT( chitBag == 0 ); chitBag = c; }
	void OnRemove()				{ GLASSERT( chitBag ); chitBag = 0; }

	void Add( Component* );
	void Remove( Component* );

	bool NeedsTick() const		{ return nTickers > 0; }
	void DoTick( U32 delta );
	void DoUpdate();

	SpatialComponent*	GetSpatialComponent()	{ return spatialComponent; }
	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }

	void RequestUpdate();

private:

	ChitBag* chitBag;
	int nTickers;	// number of components that need a tick.

	enum {
		SPATIAL,
		MOVE,
		RENDER,
		NUM_SLOTS
	};

	union {
		// Update ordering is tricky. Defined by the order of the slots;
		struct {
			SpatialComponent*	spatialComponent;
			MoveComponent*		moveComponent;
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};
};


#endif // XENOENGINE_CHIT_INCLUDED

