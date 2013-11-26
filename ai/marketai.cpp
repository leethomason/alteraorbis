#include "marketai.h"
#include "../xegame/chit.h"
#include "../game/gameitem.h" 
#include "../xegame/itemcomponent.h"

MarketAI::MarketAI( Chit* c ) : chit(c)
{
	GLASSERT( chit && chit->GetItem() && chit->GetItem()->name == "market" );
}


bool MarketAI::Has( int flag, int hardpoint, int au )
{
	ItemComponent* ic = chit->GetItemComponent();
	GLASSERT( ic );
	for( int i=1; i<ic->NumItems(); ++i ) {
		const GameItem* item = ic->GetItem( i );
		if ( !flag || (item->flags & flag) ) {
			if ( !hardpoint || (hardpoint == item->hardpoint)) {
				int value = item->GetValue();
				if ( value > 0 && value <= au ) {
					return true;
				}
			}
		}
	}
	return false;
}


bool MarketAI::HasRanged( int au )		{ return Has( GameItem::RANGED_WEAPON, 0, au ); }
bool MarketAI::HasMelee( int au )		{ return Has( GameItem::MELEE_WEAPON, 0, au ); }
bool MarketAI::HasShield( int au )		{ return Has( 0, HARDPOINT_SHIELD, au ); }

