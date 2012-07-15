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
class ChitEvent;

class IChitListener
{
public:
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* ) = 0;
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
	void DoTick( U32 delta );
	void DoUpdate();

	SpatialComponent*	GetSpatialComponent()	{ return spatialComponent; }
	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }
	Component*			GetComponent( const char* name );

	//void RequestUpdate();
	ChitBag* GetChitBag() { return chitBag; }

	/*
	enum {
		LISTENERS  = 0x01,
		COMPONENTS = 0x02,
		ALL		   = 0xff
	};
	*/

	// Send a message to the listeners, and every component
	// in the chit (which don't need to be listeners.)
	void SendMessage(	int msgID, 
						Component* exclude,			// useful to not call ourselves. 
						const ChitEvent* event );	// may be null
	void AddListener( IChitListener* listener );
	void RemoveListener( IChitListener* listener );

	// special versions for components that are weak-ref.
	void AddListener( Component* c );
	void RemoveListener( Component* c );

	void DebugStr( grinliz::GLString* str );

	// used by the spatial hash:
	Chit* next;

private:
	bool CarryMsg( int componentID, Chit* src, int msgID, const ChitEvent* event );

	ChitBag* chitBag;
	int id;
	int nTickers;	// number of components that need a tick.
	grinliz::CDynArray<IChitListener*, 2> listeners;

	struct CList
	{
		int chitID;
		int componentID;
	};
	grinliz::CDynArray<CList, 2> cListeners;

	enum {
		SPATIAL,
		MOVE,
		GENERAL_0,
		GENERAL_1,
		GENERAL_2,
		GENERAL_3,
		GENERAL_4,
		GENERAL_5,
		GENERAL_6,	// FIXME: fragile system
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
			Component*			general4;
			Component*			general5;
			Component*			general6;
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};
};


// A list presentation of chits; stores IDs, not pointers,
// so is safe to deletes.
class SafeChitList
{
public:
	SafeChitList( ChitBag* bag ) : chitBag( bag ), it( 0 )	{}
	~SafeChitList()	{}

	Chit* Add( Chit* c );		// returns chit added
	Chit* Remove( Chit* c );	// returns chit removed or null
	Chit* First() const;	
	Chit* Next() const;

	// Some or all of the chit pointers may be invalid,
	// so the structure can only return the approximate size
	int ApproxSize() const { return array.Size(); }
	
public:
	ChitBag* chitBag;
	mutable int it;
	mutable grinliz::CDynArray<int, 8> array;
};


#endif // XENOENGINE_CHIT_INCLUDED
