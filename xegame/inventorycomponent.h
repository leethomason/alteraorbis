#ifndef INVENTORY_COMPONENT_INCLUDED
#define INVENTORY_COMPONENT_INCLUDED

#include "component.h"
#include "chit.h"
#include "../grinliz/glcontainer.h"

// Doesn't isolate the game from xe-game...oh well. Fix in XenoEngine3. :)
#include "../game/gameitem.h"

/*

*/
class InventoryComponent : public Component
{
public:
	InventoryComponent( ChitBag* bag ) {}
	virtual ~InventoryComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "InventoryComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual void DebugStr( grinliz::GLString* str );

	void AddToInventory( const GameItem& item );
	void RemoveFromInventory( int slot );

	int GetItemsOfType( int flags, grinliz::CDynArray<GameItem*>* arr ) const;

	// Is carrying anything - primarily a query for the animation system.
	GameItem* IsCarrying();

private:
	// EL_MAX_METADATA is also the maximum number of hardpoints.
	// Therefore the first EL_MAX_METADATA slots are items being
	// carried, and map to the hardpoint, and slots beyond that
	// are carried in the pack.
	grinliz::CArray< GameItem, EL_MAX_METADATA > hardpoints;
	grinliz::CArray< GameItem, EL_MAX_METADATA > attached;
	grinliz::CDynArray< GameItem >				 pack;
};

#endif // INVENTORY_COMPONENT_INCLUDED