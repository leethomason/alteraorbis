#include "marketai.h"
#include "../xegame/chit.h"
#include "../game/gameitem.h" 
#include "../game/news.h"
#include "../game/reservebank.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitcontext.h"
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

	// Can always add to inventory: will be sold to reserve bank, if needed.
	// Will try to send gems to exchange first.
	if (buyer->GetItem()->wallet.Gold() >= cost)
	{
		if (doTrade) {
			if (!buyer->CanAddToInventory()) {
				const GameItem* sell = buyer->ItemToSell();
				if (!sell) return 0;
				GameItem* sold = buyer->RemoveFromInventory(sell);

				// Make room in the inventory. Give the crystal to the exchange
				// if there is one, else the reserve bank.
				const ChitContext* context = buyer->ParentChit()->Context();
				if (context && context->chitBag) {
					Vector2I sector = ToSector(buyer->ParentChit()->Position());
					Chit* exchange = context->chitBag->FindBuilding(ISC::exchange, sector, 0, LumosChitBag::EFindMode::NEAREST, 0, 0);
					Chit* vault = context->chitBag->FindBuilding(ISC::vault, sector, 0, LumosChitBag::EFindMode::NEAREST, 0, 0);

					if (exchange && exchange->GetWallet()) {
						exchange->GetWallet()->Deposit(&sold->wallet, sold->wallet);
					}
					else if (vault && vault->GetWallet()) {
						vault->GetWallet()->Deposit(&sold->wallet, sold->wallet);
					}
					else if (ReserveBank::GetWallet()) {
						ReserveBank::GetWallet()->Deposit(&sold->wallet, sold->wallet);
					}
					delete sold;
				}
			}
			GLASSERT(buyer->CanAddToInventory());

			GameItem* gi = seller->RemoveFromInventory(item);
			GLASSERT(gi);
			buyer->AddToInventory(gi);
			seller->GetItem()->wallet.Deposit(&buyer->GetItem()->wallet, cost);

			Vector2F pos = ToWorld2F(seller->ParentChit()->Position());
			Vector2I sector = ToSector(seller->ParentChit()->Position());
			GLOUTPUT(("'%s' sold to '%s' the item '%s' for %d Au at sector=%c%d\n",
				seller->ParentChit()->GetItem()->BestName(),
				buyer->ParentChit()->GetItem()->BestName(),
				gi->BestName(),
				cost,
				'A' + sector.x, 1 + sector.y));

			if (salesTax && ReserveBank::Instance()) {
				int tax = LRint(float(cost) * SALES_TAX);
				if (tax > 0) {
					salesTax->Deposit(ReserveBank::GetWallet(), tax);
					GLOUTPUT(("  tax paid: %d\n", tax));
				}
			}

			NewsHistory* history = seller->ParentChit()->Context()->chitBag->GetNewsHistory();
			history->Add(NewsEvent(NewsEvent::PURCHASED, pos, gi->ID(), buyer->ParentChit()->GetItemID(), buyer->ParentChit()->Team()));
		}
		return cost;
	}
	return 0;
}
