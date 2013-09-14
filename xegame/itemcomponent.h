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
	ItemComponent( Engine* _engine, WorldMap* map, const GameItem& _item );
	// Moves the item to this component
	ItemComponent( Engine* _engine, WorldMap* map, GameItem* item );
	virtual ~ItemComponent();

	virtual const char* Name() const { return "ItemComponent"; }
	virtual ItemComponent* ToItemComponent() { return this; }

	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s hp=%.1f/%d ", mainItem.Name(), mainItem.hp, mainItem.TotalHP() );
		if ( !wallet.IsEmpty() ) {
			str->Format( "Au=%d Cy=g%dr%db%dv%d ", wallet.gold, 
				          wallet.crystal[CRYSTAL_GREEN], wallet.crystal[CRYSTAL_RED], 
						  wallet.crystal[CRYSTAL_BLUE],  wallet.crystal[CRYSTAL_VIOLET]);
		}
	}

	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void Serialize( XStream* xs );

	virtual int DoTick( U32 delta, U32 since );
	virtual void OnChitEvent( const ChitEvent& event );

	GameItem* GetItem( int index=0 ) { return (index < itemArr.Size()) ? itemArr[index] : 0; }
	// Is carrying anything - primarily a query for the animation system.
	GameItem* IsCarrying();
	bool SwapWeapons();	// swap between the melee and ranged weapons

	// Gets the ranged weapon, optionally returns the trigger.
	IRangedWeaponItem*	GetRangedWeapon( grinliz::Vector3F* trigger );
	IMeleeWeaponItem*	GetMeleeWeapon();
	IShield*			GetShield();

	void AddGold( int delta );
	void AddGold( const Wallet& wallet );

	void EmptyWallet() { wallet.MakeEmpty(); }
	const Wallet& GetWallet() const			{ return wallet; }

	void AddToInventory( GameItem* item );

	// add XP to current item and its weapon
	void AddBattleXP( bool meleeAttack, int killshotLevel );

	// If there is a RenderComponent, bring it in sync with
	// the inventory.
	void SetHardpoints();

private:
	void DoSlowTick();
	bool EmitEffect( const GameItem& it, U32 deltaTime );
	bool ItemActive( int index );
	bool ItemActive( const GameItem* );	// expensive: needs a search.

	CTicker slowTick;
	Wallet wallet;

	Engine *engine;
	WorldMap* worldMap;

	// The itemArr and activeArr are the same size.
	// Intrinsic items are always active.
	// Some items are active just by having them (although this isn't implemented yet)
	// And some items (weapons, shields) only do something if they are the active hardpoint. The 
	// hardpoint question is the tricky one. The GameItem::hardpoint is the hardpoint desired.
	// But are we at that hardpoint? All shield resources are the same. And what if the
	// render component isn't attached? That's where order matters: the first object in the
	// array gets the hardpoints, and we get the hardpoints from the ModelResource. (So
	// we do not need the RenderComponent.)
	grinliz::CDynArray< GameItem* > itemArr;

	// The first item is what this *is*.
	// Following items are inventory: held items, intrinsic, pack.
	GameItem mainItem;	// What we are. Always first in the array.
};

#endif // ITEMCOMPONENT_INCLUDED