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
	InventoryComponent( ChitBag* bag ) : hardpoints(-1) {
		memset( intrinsicAt, 0, sizeof(GameItem*)*NUM_HARDPOINTS );
		memset( heldAt, 0, sizeof(GameItem*)*NUM_HARDPOINTS );
	}
	virtual ~InventoryComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "InventoryComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual void DebugStr( grinliz::GLString* str );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	bool AddToInventory( GameItem* item, bool equip );
	void RemoveFromInventory( int slot );

	// Is carrying anything - primarily a query for the animation system.
	GameItem* IsCarrying();
	// All weapons currently in use.
	void GetRangedWeapons( grinliz::CArray< IRangedWeaponItem*, NUM_HARDPOINTS >* weapons );
	// The current melee weapon. There is only ever 1 (or 0)
	IMeleeWeaponItem* GetMeleeWeapon();
	GameItem* GetShield();

	// Get the "chain" of items: held, intrinsic, parent.
	// Will return an empty array if the item isn't equipped.
	void GetChain( GameItem* item, grinliz::CArray< GameItem*, 4 >* chain );

	static const char* HardpointFlagToName( int f );
	static int HardpointNameToFlag( const char* name );

private:
	int hardpoints;			// which ones do we have??

	GameItem* intrinsicAt[NUM_HARDPOINTS];
	GameItem* heldAt[NUM_HARDPOINTS];
	
	grinliz::CDynArray< GameItem*, grinliz::OwnedPtrSem > freeItems;
	grinliz::CDynArray< GameItem*, grinliz::OwnedPtrSem > packItems;
};

#endif // INVENTORY_COMPONENT_INCLUDED