#include "marketai.h"
#include "../xegame/chit.h"
#include "../game/gameitem.h" 
#include "../game/news.h"
#include "../game/reservebank.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../game/lumoschitbag.h"

using namespace grinliz;

MarketAI::MarketAI( Chit* c ) : chit(c)
{
	GLASSERT( chit && chit->GetItem() && chit->GetItem()->IName() == ISC::market );
	ic = chit->GetItemComponent();
	GLASSERT( ic );
}


#if 0
// Complexity without value.
int MarketAI::ValueToCost( int value ) 
{
	if ( value == 0 ) return 0;

	int cost = int(float(value) * (1.0f + SALES_TAX));
	return cost > 1 ? cost : 1;
}


int MarketAI::ValueToTrade( int value ) 
{
/*	if ( value == 0 ) return 0;

	int cost = int( float(value) * MARKET_COST_MULT );
	return cost > 1 ? cost : 1;
	*/
	return value;
}
#endif


const GameItem* MarketAI::Has( int flag, int maxAuCost, int minAuValue )
{
	for( int i=1; i<ic->NumItems(); ++i ) {
		const GameItem* item = ic->GetItem( i );

		if (    (flag == RANGED && item->ToRangedWeapon())
			 || (flag == MELEE && item->ToMeleeWeapon())
			 || (flag == SHIELD && item->ToShield()))
		{ 
			int value = item->GetValue();
			if ( value > 0 && value >= minAuValue && value <= maxAuCost ) {
				return item;
			}
		}
	}
	return 0;
}


/*	General "transact" method because this is very tricky code to get right.
	There's always another case or bit of logic to account for.
*/
/*static*/ int MarketAI::Transact(const GameItem* item, ItemComponent* buyer, ItemComponent* seller, Wallet* salesTax, bool doTrade)
{
	GLASSERT(!item->Intrinsic());
	int cost = item->GetValue();

	if (buyer->GetItem()->wallet.Gold() >= cost	&& buyer->CanAddToInventory())
	{
		if (doTrade) {
			GameItem* gi = seller->RemoveFromInventory(item);
			GLASSERT(gi);
			buyer->AddToInventory(gi);
			seller->GetItem()->wallet.Deposit(&buyer->GetItem()->wallet, cost);

			Vector2F pos = { 0, 0 };
			Vector2I sector = { 0, 0 };
			if (seller->ParentChit()->GetSpatialComponent()) {
				pos = seller->ParentChit()->GetSpatialComponent()->GetPosition2D();
				sector = ToSector(seller->ParentChit()->GetSpatialComponent()->GetPosition2DI());
			}
			GLOUTPUT(("'%s' sold to '%s' the item '%s' for %d Au at sector=%x,%x\n",
				seller->ParentChit()->GetItem()->BestName(),
				buyer->ParentChit()->GetItem()->BestName(),
				gi->BestName(),
				cost,
				sector.x, sector.y));

			if (salesTax && ReserveBank::Instance()) {
				int tax = LRint(float(cost) * SALES_TAX);
				if (tax > 0) {
					salesTax->Deposit(ReserveBank::GetWallet(), tax);
					GLOUTPUT(("  tax paid: %d\n", tax));
				}
			}

			NewsHistory* history = seller->ParentChit()->Context()->chitBag->GetNewsHistory();
			history->Add(NewsEvent(NewsEvent::PURCHASED, pos, gi, buyer->ParentChit()));
		}
		return cost;
	}
	return 0;
}
