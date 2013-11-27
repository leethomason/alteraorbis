#ifndef MARKET_AI_INCLUDED
#define MARKET_AI_INCLUDED

class Chit;
class GameItem;
class ItemComponent;

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
	GameItem* Buy( const GameItem* itemToBuy, int* cost );

	// Returns a price greater than 0 if the market will buy this item, 0 if not
	int WillAccept( const GameItem* item );
	// Calls WillAccept() - returns >0 if purchased, 0 if not.
	int SellToMarket( GameItem* item );

private:
	// How much it cost to buy this item.
	int ValueToCost( int value ) const;
	// How much a store will pay for this item.
	int ValueToTrade( int value ) const;

	Chit* chit;
	ItemComponent* ic;
};

#endif // MARKET_AI_INCLUDED

