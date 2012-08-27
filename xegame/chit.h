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
class InventoryComponent;
class ItemComponent;

class ChitBag;
class Chit;
class ChitEvent;
class GameItem;

class ChitMsg
{
public:
	ChitMsg( int _id, int _data=0 ) : id(_id), data(_data) {}

	int ID() const { return id; }
	int Data() const { return data; }

private:
	int id;
	int data;
};



class IChitListener
{
public:
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg ) = 0;
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

	bool NeedsTick() const		{ return nTickers > 0 || oneTickNeeded; }
	void DoTick( U32 delta );

	void OnChitEvent( const ChitEvent& event );

	SpatialComponent*	GetSpatialComponent()	{ return spatialComponent; }
	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	InventoryComponent*	GetInventoryComponent()	{ return inventoryComponent; }
	ItemComponent*		GetItemComponent()		{ return itemComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }
	Component*			GetComponent( const char* name );

	ChitBag* GetChitBag() { return chitBag; }

	// Send a message to the listeners, and every component
	// in the chit (which don't need to be listeners.)
	// Synchronous
	void SendMessage(	const ChitMsg& message, 
						Component* exclude );			// useful to not call ourselves. 
	void AddListener( IChitListener* listener );
	void RemoveListener( IChitListener* listener );

	// special versions for components that are weak-ref.
	void AddListener( Component* c );
	void RemoveListener( Component* c );

	void DebugStr( grinliz::GLString* str );

	// used by the spatial hash:
	Chit* next;

private:
	bool CarryMsg( int componentID, Chit* src, const ChitMsg& msg );

	ChitBag* chitBag;
	int id;
	int		nTickers;		// number of components that need a tick.
	bool	oneTickNeeded;	// even if a component generally doesn't need a tick, it might sometimes
	U32 slowTickTimer;
	grinliz::CDynArray<IChitListener*> listeners;

	struct CList
	{
		int chitID;
		int componentID;
	};
	grinliz::CDynArray<CList> cListeners;

	enum {
		SPATIAL,
		MOVE,
		INVENTORY,
		ITEM,
		GENERAL_0,
		GENERAL_1,
		GENERAL_2,
		GENERAL_3,
		GENERAL_4,	// FIXME: fragile system
		RENDER,
		NUM_SLOTS,

		GENERAL_START = GENERAL_0,
		GENERAL_END   = RENDER,
	};

public:
	enum {
		SPATIAL_BIT		= (1<<SPATIAL),
		MOVE_BIT		= (1<<MOVE),
		INVENTORY_BIT	= (1<<INVENTORY),
		ITEM_BIT		= (1<<ITEM),
		RENDER_BIT		= (1<<RENDER)
	};

private:
	union {
		// Update ordering is tricky. Defined by the order of the slots;
		struct {
			SpatialComponent*	spatialComponent;
			MoveComponent*		moveComponent;
			InventoryComponent*	inventoryComponent;
			ItemComponent*		itemComponent;
			Component*			general0;
			Component*			general1;
			Component*			general2;
			Component*			general3;
			Component*			general4;
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};
};


struct ComponentSet
{
	enum {
		IS_ALIVE = (1<<30)
	};

	ComponentSet( Chit* chit, int bits );
	void Zero();

	bool	okay;
	Chit*	chit;
	bool	alive;

	SpatialComponent*	spatial;
	MoveComponent*		move;
	InventoryComponent*	inventory;
	ItemComponent*		itemComponent;
	GameItem*			item;
	RenderComponent*	render;
};


// A list presentation of chits; stores IDs, not pointers,
// so is safe to deletes.
class SafeChitList
{
public:
	SafeChitList( ChitBag* bag=0 ) : chitBag( bag ), it( 0 )	{}
	~SafeChitList()	{}

	void Init( ChitBag* bag );
	void Free();	// free everything, null the chitbag pointer

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
	mutable grinliz::CDynArray<int> array;
};


#endif // XENOENGINE_CHIT_INCLUDED
