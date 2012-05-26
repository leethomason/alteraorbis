#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "../xegame/xeitem.h"

class GameItem : public XEItem
{
public:
	GameItem() : steel(0), fiber(0), crystal(0)	{}
	GameItem( const char* name, const char* res ) : XEItem( name, res ), steel( 0 ), fiber( 0 ), crystal( 0 ) {}
	virtual ~GameItem()							{}

	int		steel;
	int		fiber;
	int		crystal;
};


class WeaponItem : public GameItem
{
public:
	WeaponItem() : rounds(5), roundsCap(5), rechargeTime(1000)	{}
	WeaponItem( const char* name, const char* res ) : GameItem( name, res ),
		rounds( 5 ), roundsCap(5), rechargeTime(1000) {}

	virtual ~WeaponItem()	{}

	int	rounds;
	int roundsCap;
	U32 rechargeTime;
};

#endif // GAMEITEM_INCLUDED
