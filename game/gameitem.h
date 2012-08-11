#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "gamelimits.h"

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

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

struct DamageDesc
{
	float kinetic;
	float energy;
	float fire;

	float Total() const { return kinetic + energy + fire; }
};

class IWeaponItem 
{
public:
	virtual void GetDamageDesc( DamageDesc* desc ) = 0;
};

class IMeleeWeaponItem : virtual public IWeaponItem
{};

class IRangedWeaponItem : virtual public IWeaponItem
{};


class GameItem : public IMeleeWeaponItem, public IRangedWeaponItem
{
public:
	GameItem( int _flags=0, const char* _name=0, const char* _res=0 ) :
		name( _name ),
		resource( _res ),
		flags( _flags ),
		primaryTeam( 0 ),
		rounds( 10 ),
		roundsCap( 10 ),
		reloadTime( 1000 ),
		coolDownTime( 1000 )
	{
	}

	virtual ~GameItem()	{}

	const char* Name() const			{ return name.c_str(); }
	const char* ResourceName() const	{ return resource.c_str(); }

	int AttachmentFlags() const			{ return flags & (HELD|HARDPOINT); }
	int HardpointFlags() const			{ return flags & (HARDPOINT_TRIGGER|HARDPOINT_SHIELD); }

	enum {
		// Type of the item
		CHARACTER			= (1),
		MELEE_WEAPON		= (1<<1),
		RANGED_WEAPON		= (1<<2),

		// How items are equipped. These 2 flags are much clearer as the descriptive values below.
		HELD				= (1<<3),
		HARDPOINT			= (1<<4),
		PACK				= (1<<5),	// in inventory; not carried or activated

		INTRINSIC_AT_HARDPOINT	= HARDPOINT,		//	a hand. built in, but located at a hardpoint
		INTRINSIC_FREE			= 0,				//  pincer. built in, no hardpoint
		HELD_AT_HARDPOINT		= HARDPOINT | HELD,	// 	sword, shield. at hardpoint, overrides built in.
		HELD_FREE				= HELD,				//	amulet, rind. held, put not at a hardpoint, and not rendered

		// The set of hardpoints	
		HARDPOINT_TRIGGER	= (1<<6),	// this attaches to the trigger hardpoint
		HARDPOINT_SHIELD	= (1<<7),	// this attaches to the shield hardpoint
	};

	grinliz::CStr< MAX_ITEM_NAME >		name;		// name of the item 
	grinliz::CStr< EL_RES_NAME_LEN >	resource;	// resource used to  render the item
	int flags;
	int	primaryTeam;		// who owns this items
	int rounds;
	int roundsCap;
	U32 reloadTime;			// time to set rounds back to roundsCapacity
	U32 coolDownTime;		// time between uses

	virtual IMeleeWeaponItem*	ToMeleeWeapon()		{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual IRangedWeaponItem*	ToRangedWeapon()	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual IWeaponItem*		ToWeapon()			{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }
	virtual const IWeaponItem*	ToWeapon() const	{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }

	bool Ready( U32 absTime ) {
		if ( coolDownTime == 0 || absTime >= coolDownTime ) {
			return true;
		}
		return false;
	}

	bool Use( U32 absTime ) {
		if ( Ready( absTime )) {
			coolDownTime = absTime + COOLDOWN_TIME;
			return true;
		}
		return false;
	}

	//IWeaponItem
	void GetDamageDesc( DamageDesc* desc ) {
		// FIXME: fake
		desc->energy = 20.0f;
		desc->fire = 0;
		desc->kinetic = 0;
	}
};

#endif // GAMEITEM_INCLUDED
