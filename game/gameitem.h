#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "../xegame/xeitem.h"
#include "gamelimits.h"
#include "../grinliz/glcontainer.h"

class Chit;
class WeaponItem;

class GameItem : public XEItem
{
public:
	GameItem() : steel(0), stone(0), crystal(0)	{}
	GameItem( const char* name, const char* res ) : XEItem( name, res ), steel( 0 ), stone( 0 ), crystal( 0 ) {}
	virtual ~GameItem()							{}

	// Depth=0 the chit, depth=1 1st level inventory
	static void GetActiveItems( Chit* chit, grinliz::CArray<XEItem*, MAX_ACTIVE_ITEMS>* array );

	virtual GameItem* ToGameItem() { return this; }	
	virtual WeaponItem* ToWeapon() { return 0; }

	int		steel;
	int		stone;
	int		crystal;
};


class WeaponItem : public GameItem
{
public:
	WeaponItem() : rounds(5), roundsCap(5), rechargeTime(1000)	{}
	WeaponItem( const char* name, const char* res ) : GameItem( name, res ),
		melee( true ), range( true ), rounds( 5 ), roundsCap(5), rechargeTime(1000) {}

	virtual ~WeaponItem()	{}
	virtual WeaponItem* ToWeapon() { return this; }

	bool CanMelee( U32 time ) {
		if ( melee && time >= coolTime ) {
			return true;
		}
		return false;
	}

	bool DoMelee( U32 time ) {
		if ( CanMelee( time )) {
			coolTime = time + COOLDOWN_TIME;
			return true;
		}
		return false;
	}

	bool melee;
	bool range;
	int	rounds;
	int roundsCap;
	U32 rechargeTime;
};

#endif // GAMEITEM_INCLUDED
