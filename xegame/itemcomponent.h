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

#ifndef ITEMCOMPONENT_INCLUDED
#define ITEMCOMPONENT_INCLUDED

// Hack. It begins. Engine code including game code.
#include "../game/gameitem.h"
#include "cticker.h"
#include "component.h"
#include "../engine/engine.h"
#include "../game/wallet.h"

class WorldMap;

class ItemComponent : public Component
{
private:
	typedef Component super;

public:
	// Creates an item from an item definiton.
	ItemComponent( const GameItem& _item );
	// Moves the item to this component
	ItemComponent( GameItem* item );
	virtual ~ItemComponent();

	virtual const char* Name() const { return "ItemComponent"; }
	virtual ItemComponent* ToItemComponent() { return this; }

	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();

	virtual void Serialize( XStream* xs );

	virtual int DoTick( U32 delta );
	virtual void OnChitEvent( const ChitEvent& event );

	int NumItems() const				{ return itemArr.Size(); }
	GameItem* GetItem( int index=0 )				{ return (index < itemArr.Size()) ? itemArr[index] : 0; }
	const GameItem* GetItem( int index=0 ) const	{ return ( index < itemArr.Size()) ? itemArr[index] : 0; }

	int FindItem( const GameItem* item ) const { 
		for( int i=0; i<itemArr.Size(); ++i ) {
			if ( itemArr[i] == item ) return i;
		}
		return -1;
	}
	int FindItem( const grinliz::IString& name ) const {
		for( int i=0; i<itemArr.Size(); ++i ) {
			if ( itemArr[i]->IName() == name ) return i;
		}
		return -1;
	}

	bool SwapWeapons();			// swap between the melee and ranged weapons
	bool Swap( int i, int j );	// swap 2 slots 

	static bool Swap2( ItemComponent* a, int aIndex, ItemComponent* b, int bIndex );	// swap 2 slots, in 2 (possibly) different inventoryComponents

	/*	Weapon queries. Seem simple, but aren't.
		GetRanged/Melee: returns the currently equipped weapon. Can
			be intrinsic OR held. Can be null. Can both be null.
		GetActive/Reserved: returns the a held weapon. Either in hand,
			or can be swapped in.
	*/

	// Gets the *currently* in use:
	IRangedWeaponItem*	GetRangedWeapon( grinliz::Vector3F* trigger );	// optionally returns trigger
	IMeleeWeaponItem*	GetMeleeWeapon()	{ return melee; }
	IShield*			GetShield()			{ return shield; }

	// Always returns a HELD weapon, or null.
	IWeaponItem*		GetReserveWeapon()	{ return reserve; }

	bool CanAddToInventory();
	int  NumCarriedItems() const;
	int ItemToSell() const;	// returns 0 or the cheapest item that can be sold

	// adds to the inventory; takes ownership of pointer
	void AddToInventory( GameItem* item );
	// Add the component, and deletes it.
	void AddToInventory( ItemComponent* ic );
	void AddSubInventory( ItemComponent* ic, bool addWeapons, grinliz::IString filterItems );

	// Removes an item from the inventory. Returns
	// null if that item cannot be removed.
	GameItem* RemoveFromInventory( int index );

	// drops the item from the inventory (which owns the memory)
	void Drop( const GameItem* item );

	// add XP to current item and its weapon
	void AddBattleXP( const GameItem* loser, bool killshot );
	void AddCraftXP( int nCrystals );

	// Very crude assessment of the power of this MOB.
	float PowerRating() const;

	// Normally don't call this directly, but there is some "just in case" code.
	void SortInventory();				// AIs will use the "best" item.

private:
	// If there is a RenderComponent, bring it in sync with the inventory.
	void SetHardpoints();
	// Update the active/reserve/melee/ranged. Call whenever inventory changes.
	void UpdateActive();

	void DoSlowTick();
	void ApplyLootLimits();
	bool EmitEffect( const GameItem& it, U32 deltaTime );
	bool ItemActive( int index ) const	{ return activeArr[index]; }
	bool ItemActive( const GameItem* );	// expensive: needs a search.
	void NameItem( GameItem* item );	// if conditions met, give the item a name.
	void NewsDestroy( const GameItem* item );	// generate destroy message

	// Not serialized:
	bool				hardpointsModified;
	int					lastDamageID;			// the last thing that hit us.
	IRangedWeaponItem*	ranged;		// may be 0, active, or neither
	IMeleeWeaponItem*	melee;		// may be 0, active, or neither
	IWeaponItem*		reserve;	// remember: always held
	IShield*			shield;
	CTicker				slowTick;

	// The first item in this array is what this *is*. The following items are what is being carried.
	//
	// Some items are active just by having them (although this isn't implemented yet)
	// And some items (weapons, shields) only do something if they are the active hardpoint. The 
	// hardpoint question is the tricky one. The GameItem::hardpoint is the hardpoint desired.
	// But are we at that hardpoint? All shield resources are the same. And what if the
	// render component isn't attached? That's where order matters: the first object in the
	// array gets the hardpoints, and we get the hardpoints from the ModelResource. (So
	// we do not need the RenderComponent.)
	grinliz::CDynArray< GameItem* > itemArr;
	grinliz::CDynArray< bool >		activeArr;
};

#endif // ITEMCOMPONENT_INCLUDED