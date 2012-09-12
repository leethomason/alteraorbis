#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "gamelimits.h"

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmath.h"

#include "../tinyxml2/tinyxml2.h"

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
	- An item can only attach to one hardpoint. It would be good if it could attach
	  to either left or right hands, for example.
	  
*/

class Chit;
class WeaponItem;
class GameItem;

class DamageDesc
{
public:
	enum {
		// 'shock' may be corruption, shock, or electric
		// 'energy' is a plasma bolt
		KINETIC, ENERGY, FIRE, SHOCK, NUM_COMPONENTS
	};

	typedef grinliz::MathVector<float,NUM_COMPONENTS> Vector;
	Vector components;

	DamageDesc() {}

	void Set( float _kinetic, float _energy, float _fire, float _shock ) { 
		components[KINETIC] = _kinetic;
		components[ENERGY]  = _energy;
		components[FIRE]    = _fire;
		components[SHOCK]	= _shock;
	}
	float Total() const { 
		float total=0;
		for( int i=0; i<NUM_COMPONENTS; ++i )
			total += components[i];
		return total;
	}

	float Kinetic() const	{ return components[KINETIC]; }
	float Energy() const	{ return components[ENERGY]; }
	float Fire() const		{ return components[FIRE]; }
	float Shock() const		{ return components[SHOCK]; }

	void Save( const char* prefix, tinyxml2::XMLPrinter* );
	void Load( const char* prefix, const tinyxml2::XMLElement* doc );
	void Log();
};

class IWeaponItem 
{
public:
	virtual bool Ready() = 0;
	virtual bool Use( Chit* owner ) = 0;
	virtual GameItem* GetItem() = 0;
};

class IMeleeWeaponItem : virtual public IWeaponItem
{
public:
};

class IRangedWeaponItem : virtual public IWeaponItem
{

public:
};

// FIXME: memory pool
class GameItem : private IMeleeWeaponItem, private IRangedWeaponItem
{
public:
	GameItem( int _flags=0, const char* _name=0, const char* _res=0 )
	{
		CopyFrom(0);
		flags = _flags;
		name = _name;
		resource = _res;
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

		IMMUNE_FIRE			= (1<<10),
		FLAMMABLE			= (1<<11),
		IMMUNE_ENERGY		= (1<<12),

		EFFECT_FIRE			= (1<<13),
		EFFECT_ENERGY		= (1<<14),
	};

	grinliz::CStr< MAX_ITEM_NAME >		name;		// name of the item
	grinliz::CStr< MAX_ITEM_NAME >		key;		// modified name, for storage. not serialized.
	grinliz::CStr< EL_RES_NAME_LEN >	resource;	// resource used to  render the item
	int flags;				// flags that define this item; 'constant'
	int hardpoint;
	float mass;				// mass (kg)
	float power;			// works like mass for energy items - shield strength, gun power
	int	primaryTeam;		// who owns this items
	DamageDesc meleeDamage;	// a multiplier of the base (effective mass) and other modifiers
	DamageDesc rangedDamage;// a multiplier of the power
	DamageDesc resist;		// multiplier of damage absorbed by this item. 1: normal, 0: no damage, 1.5: extra damage
	U32 cooldown;			// time between uses
	U32 coolTime;			// counting up to ready state
	U32 reload;				// time to reload once clip is used up
	int clipCap;			// possible rounds in the clip

	float hp;				// current hp for this item
	int clipCount;			// current rounds in the clip

	// Group all the copy/init in one place!
	void CopyFrom( const GameItem* rhs ) {
		if ( rhs ) {
			name			= rhs->name;
			key				= rhs->key;
			resource		= rhs->resource;
			flags			= rhs->flags;
			hardpoint		= rhs->hardpoint;
			mass			= rhs->mass;
			power			= rhs->power;
			primaryTeam		= rhs->primaryTeam;
			meleeDamage		= rhs->meleeDamage;
			rangedDamage	= rhs->rangedDamage;
			resist			= rhs->resist;
			cooldown		= rhs->cooldown;
			coolTime		= rhs->coolTime;

			hp				= rhs->hp;
		}
		else {
			name.Clear();
			key.Clear();
			resource.Clear();
			flags = 0;
			hardpoint = 0;
			mass = 1;
			power = 0;
			primaryTeam = 0;
			meleeDamage.Set( 1, 0, 0, 0 );
			rangedDamage.Set( 1, 0, 0, 0 );
			resist.Set( 1, 1, 1, 1 );	// resist nothing, bonus nothing
			cooldown = 1000;
			coolTime = cooldown;

			hp = TotalHP();
		}
	}

	virtual IMeleeWeaponItem*	ToMeleeWeapon()		{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual IRangedWeaponItem*	ToRangedWeapon()	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual IWeaponItem*		ToWeapon()			{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }
	virtual const IWeaponItem*	ToWeapon() const	{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }

	bool DoTick( U32 delta ) {
		coolTime += delta;
		return coolTime < cooldown;	
	}

	virtual bool Ready() {
		return coolTime >= cooldown;
	}

	virtual bool Use( Chit* owner );

	// Note that the current HP, if it has one, 
	float TotalHP() const { return (float) mass; }

	// Absorb damage.'remain' is how much damage passes through the shield
	void AbsorbDamage( const DamageDesc& dd, DamageDesc* remain, const char* log );

private:
};

#endif // GAMEITEM_INCLUDED
