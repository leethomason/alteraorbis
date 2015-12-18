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
	// Moves the item to this component
	ItemComponent(GameItem* item);
	virtual ~ItemComponent();
	void InitFrom(const GameItem* items[], int nItems);

	virtual const char* Name() const { return "ItemComponent"; }
	virtual ItemComponent* ToItemComponent() { return this; }

	virtual void DebugStr(grinliz::GLString* str);
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);
	virtual void OnAdd(Chit* chit, bool init);
	virtual void OnRemove();

	virtual void Serialize(XStream* xs);

	virtual int DoTick(U32 delta);
	virtual void OnChitEvent(const ChitEvent& event);

	int NumItems() const							{ return itemArr.Size(); }
	int NumItems(const grinliz::IString& istr);
	GameItem* GetItem()								{ return itemArr[0]; }
	const GameItem* GetItem(int index = 0) const	{ return (index < itemArr.Size()) ? itemArr[index] : 0; }

	const GameItem* FindItem(const GameItem* item) const {
		for (int i = 0; i < itemArr.Size(); ++i) {
			if (itemArr[i] == item) return item;
		}
		return 0;
	}
	const GameItem* FindItem(const grinliz::IString& name) const {
		for (int i = 0; i < itemArr.Size(); ++i) {
			if (itemArr[i]->IName() == name) return itemArr[i];
		}
		return 0;
	}

	enum { SELECT_MELEE, SELECT_RANGED };
	const GameItem* SelectWeapon(int type);
	const GameItem* QuerySelectWeapon(int type) const;	// queries what will be swapped to, but doesn't swap
	const RangedWeapon* QuerySelectRanged() const;		// queries ranged, but doesn't swap
	const MeleeWeapon* QuerySelectMelee() const;		// queries melee, but doesn't swap

	bool IsBetterItem(const GameItem* item) const;		// return true if 'item' is better than the one this has

	bool Swap(int i, int j);	// swap 2 slots 

	static bool Swap2(ItemComponent* a, int aIndex, ItemComponent* b, int bIndex);	// swap 2 slots, in 2 (possibly) different inventoryComponents

	/*	Weapon queries. Seems simple to do...but wow I have re-written this code.
		Gets the *active* weapon. Can be Intrinsic or Held.
		Can be null.
		*/
	RangedWeapon*	GetRangedWeapon(grinliz::Vector3F* trigger) const;	// optionally returns trigger
	MeleeWeapon*	GetMeleeWeapon() const;
	Shield*			GetShield() const;

	void SetMaxCarried(int maxInventory) { maxCarriedItems = maxInventory; }
	bool CanAddToInventory();
	int  NumCarriedItems() const;
	int  NumCarriedItems(const grinliz::IString& str) const;

	const GameItem* ItemToSell() const;	// returns 0 or the cheapest item that can be sold

	// adds to the inventory; takes ownership of pointer
	void AddToInventory( GameItem* item );
	// Add the component, and deletes it.
	void AddToInventory( ItemComponent* ic );
	// Move from one inventory to another. Return count of items moved.
	int TransferInventory( ItemComponent* ic, bool addWeapons, grinliz::IString filterItems );
	// Sell, if possible, to make room.
	void MakeRoomInInventory();

	// Removes an item from the inventory. Returns
	// null if that item cannot be removed.
	GameItem* RemoveFromInventory( const GameItem* );

	// drops the item from the inventory (which owns the memory)
	void Drop( const GameItem* item );

	// add XP to current item and its weapon
	void AddBattleXP( const GameItem* loser, bool killshot );
	void AddCraftXP( int nCrystals );

	// Very crude assessment of the power of this MOB.
	// If current=true, uses the current HP vs. Total
	int PowerRating(bool current) const;

	void EnableDebug(bool d) { debugEnabled = d;  }
	int LastDamageID() const { return lastDamageID;  }

private:
	// If there is a RenderComponent, bring it in sync with the inventory.
	void SetHardpoints();
	void UseBestItems();
	void SortInventory();				// AIs will use the "best" item.
	void Validate() const;
	void ComputeHardpoints();

	void DoSlowTick();
	void ApplyLootLimits();
	bool EmitEffect( const GameItem& it, U32 deltaTime );
	bool ItemActive( const GameItem* );
	void NameItem( GameItem* item );	// if conditions met, give the item a name.
	void NewsDestroy( const GameItem* item );	// generate destroy message
	int ProcessEffect( int delta);	// look around for environment that effects this itemComp, and apply those effects
	void InformCensus(bool add);

	// Not serialized:
	bool		hardpointsModified;
	grinliz::CArray<GameItem*, EL_NUM_METADATA>	hardpoint;	// a CArray for the search, etc.
	bool		hasHardpoint[EL_NUM_METADATA];
	CTicker		slowTick;
	int			lastDamageID;
	bool		debugEnabled;
	bool		hardpointsComputed;
	int			maxCarriedItems;

	// The first item in this array is what this *is*. The following items are what is being carried.
	grinliz::CDynArray< GameItem* > itemArr;
};

#endif // ITEMCOMPONENT_INCLUDED