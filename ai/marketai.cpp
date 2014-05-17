#include "marketai.h"
#include "../xegame/chit.h"
#include "../game/gameitem.h" 
#include "../game/news.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

MarketAI::MarketAI( Chit* c ) : chit(c)
{
	GLASSERT( chit && chit->GetItem() && chit->GetItem()->IName() == ISC::market );
	ic = chit->GetItemComponent();
	GLASSERT( ic );
}


/*static*/ int MarketAI::ValueToCost( int value ) 
{
	if ( value == 0 ) return 0;

	int cost = int(float(value) * (1.0f + SALES_TAX));
	return cost > 1 ? cost : 1;
}


/*static*/ int MarketAI::ValueToTrade( int value ) 
{
/*	if ( value == 0 ) return 0;

	int cost = int( float(value) * MARKET_COST_MULT );
	return cost > 1 ? cost : 1;
	*/
	return value;
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
const GameItem* MarketAI::HasMelee( int au, int minValue )	{ return Has( GameItem::MELEE_WEAPON, 0, au, minValue ); }
const GameItem* MarketAI::HasShield( int au, int minValue )	{ return Has( 0, HARDPOINT_SHIELD, au, minValue ); }


/*	General "transact" method because this is very tricky code to get right.
	There's always another case or bit of logic to account for.
*/
/*static*/ int MarketAI::Transact( const GameItem* itemToBuy, ItemComponent* buyer, ItemComponent* seller, Wallet* salesTax, bool doTrade )
{
	int cost = 0;
	for( int i=1; i<seller->NumItems(); ++i ) {
		if ( !seller->GetItem(i)->Intrinsic() ) {
			if ( seller->GetItem(i) == itemToBuy ) {
				cost = ValueToCost( itemToBuy->GetValue() );
				int tax = cost - itemToBuy->GetValue();

				if (    buyer->GetItem()->wallet.gold >= cost
					 && buyer->CanAddToInventory() ) 
				{
					if ( doTrade ) {
						GameItem* gi = seller->RemoveFromInventory( i );
						GLASSERT( gi );
						buyer->AddToInventory( gi );
						Transfer(&seller->GetItem()->wallet, &buyer->GetItem()->wallet, cost);

						Vector2F pos    = { 0, 0 };
						Vector2I sector = { 0, 0 };
						if ( seller->ParentChit()->GetSpatialComponent() ) {
							pos = seller->ParentChit()->GetSpatialComponent()->GetPosition2D();
							sector = ToSector( seller->ParentChit()->GetSpatialComponent()->GetPosition2DI() );
						}
						GLOUTPUT(( "'%s' sold to '%s' the item '%s' for %d Au at sector=%x,%x\n",
							seller->ParentChit()->GetItem()->BestName(),
							buyer->ParentChit()->GetItem()->BestName(),
							gi->BestName(),
							cost,
							sector.x, sector.y ));

						if (tax && salesTax) {
							GLASSERT(seller->GetItem()->wallet.gold > tax);
							Transfer(salesTax, &seller->GetItem()->wallet, tax);
							GLOUTPUT(("  tax paid: %d\n", tax));
						}

						NewsHistory* history = seller->ParentChit()->GetChitBag()->GetNewsHistory();
						history->Add( NewsEvent( NewsEvent::PURCHASED, pos, gi, buyer->ParentChit() ));
					}
					return cost;
				}
			}
		}
	}
	return 0;
}
