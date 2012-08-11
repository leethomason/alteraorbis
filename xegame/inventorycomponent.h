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

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	void AddToInventory( const GameItem& item, bool equip );
	void RemoveFromInventory( int slot );

	int GetItemsOfType( int flags, grinliz::CDynArray<GameItem*>* arr ) const;

	// Is carrying anything - primarily a query for the animation system.
	GameItem* IsCarrying();

	// Weapon in/is hand first. Followed by intrinsics.
	void GetWeapons( grinliz::CArray< GameItem*, EL_MAX_METADATA >* weapons );

	static const char* HardpointFlagToName( int f );
	static int HardpointNameToFlag( const char* name );

private:
	int hardpoints;
	int hardpointsInUse;
	grinliz::CDynArray< GameItem > equippedItems;
	grinliz::CDynArray< GameItem > packItems;
};

#endif // INVENTORY_COMPONENT_INCLUDED