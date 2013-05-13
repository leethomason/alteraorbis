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
	ItemComponent( Engine* _engine, WorldMap* map, const GameItem& _item );
	virtual ~ItemComponent();

	virtual const char* Name() const { return "ItemComponent"; }
	virtual ItemComponent* ToItemComponent() { return this; }

	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s hp=%.1f/%.1f ", mainItem.Name(), mainItem.hp, mainItem.TotalHP() );
		if ( !wallet.IsEmpty() ) {
			str->Format( "Au=%d Cy=r%dg%dv%d ", wallet.gold, wallet.crystal[CRYSTAL_RED], wallet.crystal[CRYSTAL_GREEN], wallet.crystal[CRYSTAL_VIOLET]);
		}
	}

	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void Serialize( XStream* xs );

	virtual int DoTick( U32 delta, U32 since );
	virtual void OnChitEvent( const ChitEvent& event );

	GameItem* GetItem() { return &mainItem; }
	// Is carrying anything - primarily a query for the animation system.
	GameItem* IsCarrying();

	// Gets the ranged weapon, optionally returns the trigger.
	IRangedWeaponItem*	GetRangedWeapon( grinliz::Vector3F* trigger );
	IMeleeWeaponItem*	GetMeleeWeapon();
	IShield*			GetShield();

	void AddGold( int delta );
	void AddGold( const Wallet& wallet );

	void EmptyWallet() { wallet.MakeEmpty(); }
	const Wallet& GetWallet() const			{ return wallet; }

	enum {
		NO_AUTO_PICKUP,
		GOLD_PICKUP,
		GOLD_HOOVER
	};
	void SetPickup( int mode )	{ pickupMode = mode; }
	int Pickup() const			{ return pickupMode; }

	bool AddToInventory( GameItem* item, bool equip );

	// add XP to current item and its weapon
	void AddBattleXP( bool meleeAttack, int killshotLevel );

private:
	void DoSlowTick();
	bool EmitEffect( const GameItem& it, U32 deltaTime );

	CTicker slowTick;
	Wallet wallet;
	int pickupMode;

	Engine *engine;
	WorldMap* worldMap;
	// The first item is what this *is*.
	// Following items are inventory: held items, intrinsic, pack.
	GameItem mainItem;	// What we are. Always first in the array.
	grinliz::CDynArray< GameItem* > itemArr;
};

#endif // ITEMCOMPONENT_INCLUDED