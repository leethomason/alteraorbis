#ifndef XENOENGINE_CHIT_INCLUDED
#define XENOENGINE_CHIT_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

class Component;
class SpatialComponent;
class RenderComponent;
class MoveComponent;

class ChitBag;
class Chit;

class IChitListener
{
public:
	virtual void OnChitMsg( Chit* chit, const char* componentName, int id ) = 0;
};

// MyComponent* mc = GET_COMPONENT( chit, MyComponent );
#define GET_COMPONENT( chit, name ) static_cast<name*>( chit->GetComponent( #name ) )


/* General purpose GameObject.
   A class to hold Components.
	
*/
class Chit
{
public:
	// Should always use Create from ChitBag
	Chit( int id, ChitBag* chitBag );
	// Should always use Delete from ChitBag
	~Chit();

	int ID() const { return id; }
	void Add( Component* );
	void Remove( Component* );

	bool NeedsTick() const		{ return nTickers > 0; }
	// return if this is current DoTick processing - useful to not send uneeded updates.
	bool Ticking() const		{ return ticking; }

	void DoTick( U32 delta );
	void DoUpdate();

	SpatialComponent*	GetSpatialComponent()	{ return spatialComponent; }
	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }
	Component*			GetComponent( const char* name );

	void RequestUpdate();
	ChitBag* GetChitBag() { return chitBag; }

	void SendMessage( const char* componentName, int id );
	void AddListener( IChitListener* listener );
	void RemoveListener( IChitListener* listener );

	void DebugStr( grinliz::GLString* str );

	// used by the spatial hash:
	Chit* next;

private:

	ChitBag* chitBag;
	int id;
	int nTickers;	// number of components that need a tick.
	bool ticking;
	grinliz::CDynArray<IChitListener*, 4> listeners;

	enum {
		SPATIAL,
		MOVE,
		GENERAL_0,
		GENERAL_1,
		GENERAL_2,
		GENERAL_3,
		RENDER,
		NUM_SLOTS,

		GENERAL_START = GENERAL_0,
		GENERAL_END   = RENDER,
	};

	union {
		// Update ordering is tricky. Defined by the order of the slots;
		struct {
			SpatialComponent*	spatialComponent;
			MoveComponent*		moveComponent;
			Component*			general0;
			Component*			general1;
			Component*			general2;
			Component*			general3;
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};
};


#endif // XENOENGINE_CHIT_INCLUDED

