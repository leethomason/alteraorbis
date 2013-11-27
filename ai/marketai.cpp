#include "marketai.h"
#include "../xegame/chit.h"
#include "../game/gameitem.h" 
#include "../xegame/itemcomponent.h"

MarketAI::MarketAI( Chit* c ) : chit(c)
{
	GLASSERT( chit && chit->GetItem() && chit->GetItem()->name == "market" );
	ic = chit->GetItemComponent();
	GLASSERT( ic );
}


int MarketAI::ValueToCost( int value ) const 
{
	if ( value == 0 ) return 0;

	int cost = int( float(value) / MARKET_COST_MULT );
	return cost > 1 ? cost : 1;
}


int MarketAI::ValueToTrade( int value ) const 
{
	if ( value == 0 ) return 0;

	int cost = int( float(value) * MARKET_COST_MULT );
	return cost > 1 ? cost : 1;
}


const GameItem* MarketAI::Has( int flag, int hardpoint, int maxAuCost, int minAuValue )
{
	for( int i=1; i<ic->NumItems(); ++i ) {
		const GameItem* item = ic->GetItem( i );
		if ( !flag || (item->flags & flag) ) {
			if ( !hardpoint || (hardpoint == item->hardpoint)) {
				int value = item->GetValue();
				int cost = ValueToCost( value );
				if ( value > 0 && value >= minAuValue && cost <= maxAuCost ) {
					return item;
				}
			}
		}
	}
	return 0;
}


const GameItem* MarketAI::HasRanged( int au, int minValue )	{ return Has( GameItem::RANGED_WEAPON, 0, au, minValue ); }
const GameItem* MarketAI::HasMelee( int au, int minValue )		{ return Has( GameItem::MELEE_WEAPON, 0, au, minValue ); }
const GameItem* MarketAI::HasShield( int au, int minValue )	{ return Has( 0, HARDPOINT_SHIELD, au, minValue ); }


GameItem* MarketAI::Buy( const GameItem* itemToBuy, int* cost )
{
	*cost = 0;
	for( int i=1; i<ic->NumItems(); ++i ) {
		if ( ic->GetItem(i) == itemToBuy ) {
			GameItem* item = ic->RemoveFromInventory( i );
			*cost = ValueToCost( item->GetValue() );
			return item;
		}
	}
	return 0;
}


int MarketAI::WillAccept( const GameItem* item )
{
	if ( !item ) return 0;
	if ( !ic->CanAddToInventory() ) return 0;

	int value = item->GetValue();
	if ( value == 0 ) return 0;

	int cost = ValueToTrade( value );
	// Will buy anything that the market can affort.
	// Questionable whether this is the best policy.
	if ( cost <= ic->GetItem()->wallet.gold )
		return cost;
	return 0;
}


int MarketAI::SellToMarket( GameItem* item )
{
	int cost = WillAccept( item );
	if ( cost ) {
		ic->AddToInventory( item );
		ic->GetItem()->wallet.AddGold( -cost );
		return cost;
	}
	return 0;
}

