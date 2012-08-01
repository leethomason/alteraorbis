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
	- Some are built in: hands, claws, pincers. These things are hardpoints.
	- Some attach to hardpoints: shields
	- Some over-ride the built in AND attach to hardpoints: gun, sword
	- Some are carried in the back pack
	
	Constraints:
	- There can only be one melee weapon / attack. This simplies the animation,
	  and not having to track which melee hit. (This could be fixed, of course.)
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
	GameItem( int _flags=0, const char* _name=0, const char* _res=0, const char* _hardpoint=0 ) :
		name( _name ),
		resource( _res ),
		hardpoint( _hardpoint ),
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
	const char* Hardpoint() const		{ return hardpoint.c_str(); }

	enum {
		CHARACTER			= (1),
		MELEE_WEAPON		= (1<<1),
		RANGED_WEAPON		= (1<<2),

		APPENDAGE			= (1<<3),	// this is built in: hands, claws, etc. Appendage is a hard point.
		HELD				= (1<<4),	// requires an appendage to use, and overrides that appendage
	};

	grinliz::CStr< MAX_ITEM_NAME >		name;
	grinliz::CStr< EL_RES_NAME_LEN >	resource;
	grinliz::CStr< EL_RES_NAME_LEN >	hardpoint;
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
