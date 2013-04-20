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

class ItemComponent : public Component
{
private:
	typedef Component super;

public:
	ItemComponent( Engine* _engine, const GameItem& _item );
	virtual ~ItemComponent();

	virtual const char* Name() const { return "ItemComponent"; }

	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s hp=%.1f/%.1f ", mainItem.Name(), mainItem.hp, mainItem.TotalHP() );
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

	bool AddToInventory( GameItem* item, bool equip );

private:
	void DoSlowTick();
	bool EmitEffect( const GameItem& it, U32 deltaTime );

	CTicker slowTick;

	Engine *engine;
	// The first item is what this *is*.
	// Following items are inventory: held items, intrinsic, pack.
	GameItem mainItem;	// What we are. Always first in the array.
	grinliz::CDynArray< GameItem* > itemArr;
};

#endif // ITEMCOMPONENT_INCLUDED