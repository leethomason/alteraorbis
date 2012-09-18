/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

// Synchronous. Can be sub-classed.
class ChitMsg
{
public:
	enum {
		// ---- Chit --- //
		CHIT_DESTROYED,			// sender: health
		CHIT_DAMAGE,			// sender: chitBag, Ptr = &DamageDesc

		// ---- Component ---- //
		// Game
		PATHMOVE_DESTINATION_REACHED,
		PATHMOVE_DESTINATION_BLOCKED,

		HEALTH_CHANGED,
		
		ITEM_ROUNDS_CHANGED,	// Ptr = &GameItem
		ITEM_RELOADING,			// Ptr = &GameItem		FIXME: high frequency

		// XE
		SPATIAL_CHANGED,		// the position or rotation has changed, sender: spatial
		RENDER_IMPACT,			// impact metadata event has occured, sender: render component
	};

	ChitMsg( int _id, int _data=0, const void* _ptr=0 ) : id(_id), data(_data), ptr(_ptr) {}

	int ID() const { return id; }
	int Data() const { return data; }
	const void* Ptr() const { return ptr; }

private:
	int id;
	int data;
	const void* ptr;
};



class IChitListener
{
public:
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg ) = 0;
};

// MyComponent* mc = GET_COMPONENT( chit, MyComponent );
#define GET_COMPONENT( chit, name ) static_cast<name*>( chit->GetComponent( #name ) )


/*	General purpose GameObject.
	A class to hold Components.
	Actually a POD. Do not subclass.
*/
class Chit
{
public:
	// Should always use Create from ChitBag
	Chit( int id=0, ChitBag* chitBag=0 );
	// Should always use Delete from ChitBag
	~Chit();

	void Init( int id, ChitBag* chitBag );
	void Free();

	int ID() const { return id; }
	void Add( Component* );
	void Remove( Component* );

	bool TickNeeded() const		{ return tickNeeded; }
	void SetTickNeeded()		{ tickNeeded = true; }
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
	int		 id;
	bool	 tickNeeded;
	U32		 slowTickTimer;
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
