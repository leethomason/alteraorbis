#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "../xegame/xeitem.h"
#include "gamelimits.h"
#include "../grinliz/glcontainer.h"

class Chit;

class GameItem : public XEItem
{
public:
	GameItem() : steel(0), stone(0), crystal(0)	{}
	GameItem( const char* name, const char* res ) : XEItem( name, res ), steel( 0 ), stone( 0 ), crystal( 0 ) {}
	virtual ~GameItem()							{}

	// Depth=0 the chit, depth=1 1st level inventory
	static void GetActiveItems( Chit* chit, grinliz::CArray<XEItem*, MAX_ACTIVE_ITEMS>* array );

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

	bool melee;
	bool range;
	int	rounds;
	int roundsCap;
	U32 rechargeTime;
};

#endif // GAMEITEM_INCLUDED
