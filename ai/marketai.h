#ifndef MARKET_AI_INCLUDED
#define MARKET_AI_INCLUDED

class Chit;
class GameItem;
class ItemComponent;
struct Wallet;

/* utility class to interpret a chit as a market. */
class MarketAI
{
public:
	MarketAI( Chit* chit );

	const GameItem* HasRanged( int maxAuCost, int minAuValue=1 );
	const GameItem* HasMelee(  int maxAuCost, int minAuValue=1 );
	const GameItem* HasShield( int maxAuCost, int minAuValue=1 );

	// generic form of the query
	const GameItem* Has( int flag, int hardpoint, int maxAuCost, int minAuValue );

	// returns cost (>0) if able to buy
	static int Transact(	const GameItem* itemToBuy, 
							ItemComponent* buyer,
							ItemComponent* seller,
							Wallet* salesTax,
							bool doTrade );					// If 'false', no transaction is done, but returns the cost IF the trade would succeed.

	// How much it cost to buy this item.
	static int ValueToCost( int value );
	// How much a store will pay for this item.
	static int ValueToTrade( int value );

private:
	Chit* chit;
	ItemComponent* ic;
};

#endif // MARKET_AI_INCLUDED

