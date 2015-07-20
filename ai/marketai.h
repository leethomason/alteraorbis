#ifndef MARKET_AI_INCLUDED
#define MARKET_AI_INCLUDED

class Chit;
class GameItem;
class ItemComponent;
class Wallet;
class CoreScript;

/* utility class to interpret a chit as a market. */
class MarketAI
{
public:
	MarketAI(Chit* chit);

	const GameItem* HasRanged(int maxAuCost, int minAuValue = 1)	{ return Has(RANGED, maxAuCost, minAuValue); }
	const GameItem* HasMelee(int maxAuCost, int minAuValue = 1)		{ return Has(MELEE, maxAuCost, minAuValue); }
	const GameItem* HasShield(int maxAuCost, int minAuValue = 1)	{ return Has(SHIELD, maxAuCost, minAuValue); }

	// generic form of the query
	enum { RANGED, MELEE, SHIELD };
	const GameItem* Has(int flag, int maxAuCost, int minAuValue);

	// returns cost (>0) if able to buy
	static int Transact(const GameItem* itemToBuy,
						ItemComponent* buyer,
						ItemComponent* seller,
						Wallet* salesTax,
						bool doTrade);					// If 'false', no transaction is done, but returns the cost IF the trade would succeed.

	static bool CoreHasMarket(CoreScript* cs);
	static bool CoreHasMarket(CoreScript* cs, Chit* forThisChit);

private:
	Chit* chit;
	ItemComponent* ic;
};

#endif // MARKET_AI_INCLUDED

