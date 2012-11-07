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

#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "gamelimits.h"

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmath.h"

#include "../tinyxml2/tinyxml2.h"

class DamageDesc;

/*
	Items and Inventory.

	Almost everything *is* an item. Characters, walls, plants, guns all
	have properties, health, etc. Lots of things are items, and therefore
	have a GameItem in an ItemComponent.

	Characters (and some other things) *have* items, in an inventory.
	- Some are built in: hands, claws, pincers. These may be hardpoints.
	- Some attach to hardpoints: shields
	- Some over-ride the built in AND attach to hardpoints: gun, sword
	- Some are carried in the pack
		
		Examples:
			- Human
				- "hand" is an item and trigger hardpoint.
				- "sword" overrides hand and attaches to trigger hardpoint
				- "shield" attaches to the shield harpoint (but there is no
				   item that it overrides)
				- "amulet" is carried, doesn't render, but isn't in pack.
			- Mantis
				- "pincer" is an item, but not a hardpoint

		Leads to:
			INTRINSIC_AT_HARDPOINT		hand
			INTRINSIC_FREE				pincer
			HELD_AT_HARDPOINT			sword, shield
			HELD_FREE					amulet

	Hardpoints
	- The possible list of hardpoints lives in the GameItem (as bit flags)
	- The Render component exposes a list of metadata is supports
	- The Inventory component translates between the metadata and hardpoints

	Constraints:
	- There can only be one melee weapon / attack. This simplies the animation,
	  and not having to track which melee hit. (This could be fixed, of course.)
	  The melee weapon must be on the 'trigger' hardpoint.
	- Similarly, the shield must be on the 'shield' hardpoitn
	- An item can only attach to one hardpoint. It would be good if it could attach
	  to either left or right hands, for example.
	  
*/

class Chit;
class WeaponItem;
class GameItem;


class DamageDesc
{
public:
	DamageDesc() : damage(1), effects(0) {}
	DamageDesc( float _damage, int _effects ) : damage(_damage), effects(_effects) {}

	float damage;
	int   effects;

//	bool Kinetic() const	{ return (effects & GameItem::EFFECT_ENERGY) == 0; }
//	bool Energy() const		{ return (effects & GameItem::EFFECT_ENERGY) != 0; }
//	bool Fire() const		{ return (effects & GameItem::EFFECT_FIRE) != 0; }
//	bool Shock() const		{ return (effects & GameItem::EFFECT_SHOCK) != 0; }
//	bool Explosive() const	{ return (effects & GameItem::EFFECT_EXPLOSIVE) != 0; }

//	void Save( const ch//ar* prefix, tinyxml2::XMLPrinter* );
//	void Load( const char* prefix, const tinyxml2::XMLElement* doc );
	void Log();
};


class IWeaponItem 
{
public:
	virtual bool Ready() const = 0;
	virtual bool Use() = 0;
	virtual GameItem* GetItem() = 0;
	virtual bool Reload() = 0;
	virtual bool CanReload() const = 0;
	virtual bool Reloading() const = 0;
};

class IMeleeWeaponItem : virtual public IWeaponItem
{
public:
};

class IRangedWeaponItem : virtual public IWeaponItem
{
public:
};

class IShield
{
public:
};


// FIXME: memory pool
class GameItem :	private IMeleeWeaponItem, 
					private IRangedWeaponItem,
					private IShield
{
public:
	GameItem( int _flags=0, const char* _name=0, const char* _res=0 )
	{
		CopyFrom(0);
		flags = _flags;
		name = grinliz::StringPool::Intern( _name );
		resource = grinliz::StringPool::Intern( _res );
	}

	GameItem( const GameItem& rhs )			{ CopyFrom( &rhs );	}
	void operator=( const GameItem& rhs )	{ CopyFrom( &rhs );	}

	virtual ~GameItem()	{}
	virtual GameItem* GetItem() { return this; }

	virtual void Save( tinyxml2::XMLPrinter* );
	virtual void Load( const tinyxml2::XMLElement* doc );
	// If an intrinsic sub item has a trait - say, FIRE - that
	// implies that the parent is immune to fire. Apply() sets
	// basic sanity flags.
	void Apply( const GameItem* intrinsic );	

	const char* Name() const			{ return name.c_str(); }
	const char* ResourceName() const	{ return resource.c_str(); }

	int AttachmentFlags() const			{ return flags & (HELD|HARDPOINT); }

	enum {
		// Type(s) of the item
		CHARACTER			= (1),
		MELEE_WEAPON		= (1<<1),
		RANGED_WEAPON		= (1<<2),

		// How items are equipped. These 2 flags are much clearer as the descriptive values below.
		HELD				= (1<<3),
		HARDPOINT			= (1<<4),
		PACK				= (1<<5),	// in inventory; not carried or activated

		// ALL weapons have to be at hardpoints, for effect rendering if nothing else.
		// Also the current weapons are scanned from a fixed array of hardpoints.
		// May be able to pull out the "FREE" case.
		INTRINSIC_AT_HARDPOINT	= HARDPOINT,		//	a hand. built in, but located at a hardpoint
		INTRINSIC_FREE			= 0,				//  pincer. built in, no hardpoint
		HELD_AT_HARDPOINT		= HARDPOINT | HELD,	// 	sword, shield. at hardpoint, overrides built in.
		HELD_FREE				= HELD,				//	amulet, ring. held, put not at a hardpoint, and not rendered

		IMMUNE_FIRE			= (1<<6),				// doesn't burn *at all*
		FLAMMABLE			= (1<<7),				// burns until gone (wood)
		IMMUNE_SHOCK		= (1<<8),
		SHOCKABLE			= (1<<9),

		EFFECT_KINETIC		= (1<<10),
		EFFECT_ENERGY		= (1<<11),
		EFFECT_EXPLOSIVE	= (1<<12),
		EFFECT_FIRE			= (1<<13),
		EFFECT_SHOCK		= (1<<14),
		EFFECT_MASK			= EFFECT_KINETIC | EFFECT_ENERGY | EFFECT_EXPLOSIVE | EFFECT_FIRE | EFFECT_SHOCK,

		RENDER_TRAIL		= (1<<15),				// render a bolt with a 'smoketrail' vs. regular bolt
	};

	// ------ description ------
	grinliz::IString		name;		// name of the item
	grinliz::IString		key;		// modified name, for storage. not serialized.
	grinliz::IString		resource;	// resource used to  render the item
	int flags;				// flags that define this item; 'constant'
	int hardpoint;			// id of hardpoint this item attaches to
	float mass;				// mass (kg)
	float hpPerMass;		// typically 1
	float hpRegen;			// hp / second regenerated (or lost) by this item
	int	primaryTeam;		// who owns this items
	float meleeDamage;		// a multiplier of the base (effective mass) and other modifiers
	float rangedDamage;		// base ranged damage
	float absorbsDamage;	// how much damage this consumes, in the inventory (shield, armor, etc.) 1.0: all, 0.5: half
	U32 cooldown;			// time between uses
	U32 reload;				// time to reload once clip is used up
	int clipCap;			// possible rounds in the clip
	float speed;			// speed for a variety of uses. 

	// ------- current --------
	float hp;				// current hp for this item
	U32 cooldownTime;		// counting UP to ready state
	U32 reloadTime;			// counting UP to ready state
	int rounds;				// current rounds in the clip
	float accruedFire;		// how much fire damage built up, not yet applied
	float accruedShock;		// how much shock damage built up, not yet applied

	Chit* parentChit;		// only set when attached to a Component

	// Group all the copy/init in one place!
	void CopyFrom( const GameItem* rhs ) {
		if ( rhs ) {
			name			= rhs->name;
			key				= rhs->key;
			resource		= rhs->resource;
			flags			= rhs->flags;
			hardpoint		= rhs->hardpoint;
			mass			= rhs->mass;
			hpPerMass		= rhs->hpPerMass;
			hpRegen			= rhs->hpRegen;
			primaryTeam		= rhs->primaryTeam;
			meleeDamage		= rhs->meleeDamage;
			rangedDamage	= rhs->rangedDamage;
			absorbsDamage	= rhs->absorbsDamage;
			cooldown		= rhs->cooldown;
			cooldownTime	= rhs->cooldownTime;
			reload			= rhs->reload;
			reloadTime		= rhs->reloadTime;
			clipCap			= rhs->clipCap;
			rounds			= rhs->rounds;
			speed			= rhs->speed;

			hp				= rhs->hp;
			accruedFire		= rhs->accruedFire;
			accruedShock	= rhs->accruedShock;

			parentChit		= 0;	// NOT copied
		}
		else {
			name = grinliz::IString();
			key  = grinliz::IString();
			resource = grinliz::IString();
			flags = 0;
			hardpoint = 0;
			mass = 1;
			hpPerMass = 1;
			hpRegen = 0;
			primaryTeam = 0;
			meleeDamage = 1;
			rangedDamage = 0;
			absorbsDamage = 0;
			cooldown = 1000;
			cooldownTime = cooldown;
			reload = 2000;
			reloadTime = reload;
			clipCap = 0;			// default to no clip and unlimited ammo
			rounds = 0;
			speed = 1.0f;

			hp = TotalHP();
			accruedFire = 0;
			accruedShock = 0;

			parentChit = 0;
		}
	}

	virtual IMeleeWeaponItem*	ToMeleeWeapon()		{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual IRangedWeaponItem*	ToRangedWeapon()	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual IWeaponItem*		ToWeapon()			{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }
	virtual const IWeaponItem*	ToWeapon() const	{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }

	virtual IShield*			ToShield()			{ return ( hardpoint == HARDPOINT_SHIELD ) ? this : 0; }

	int Effects() const { return flags & EFFECT_MASK; }
	bool DoTick( U32 delta );
	
	// 'Ready' and 'Rounds' are orthogonal. You are Ready()
	// if the cooldown is passed. You HasRound() if the
	// weapon has (possibly unlimited) rounds.
	virtual bool Ready() const {
		return cooldownTime >= cooldown;
	}

	virtual bool Use();
	virtual bool Reload();
	virtual bool CanReload() const { return Ready() && !Reloading() && (rounds < clipCap); }
	virtual bool Reloading() const { return clipCap > 0 && reloadTime < reload; }

	int Rounds() const { return rounds; }
	int ClipCap() const { return clipCap; }
	bool HasRound() const { 
		GLASSERT( rounds <= clipCap );
		return rounds || clipCap == 0; 
	}
	void UseRound();
	bool OnFire() const  { return (!(flags & IMMUNE_FIRE)) && accruedFire > 0; }
	bool OnShock() const { return (!(flags & IMMUNE_SHOCK)) && accruedShock > 0; }

	// Note that the current HP, if it has one, 
	float TotalHP() const { return mass*hpPerMass; }
	float RoundsFraction() const {
		if ( clipCap ) {
			return (float)rounds / (float)clipCap;
		}
		return 1.0f;
	}

	// Absorb damage.'remain' is how much damage passes through the shield
	void AbsorbDamage( bool inInventory, const DamageDesc& dd, DamageDesc* remain, const char* log );

private:
	float Delta( U32 delta, float v ) {
		return v * (float)delta * 0.001f;
	}
};



#endif // GAMEITEM_INCLUDED
