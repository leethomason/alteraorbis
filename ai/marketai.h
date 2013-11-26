#ifndef MARKET_AI_INCLUDED
#define MARKET_AI_INCLUDED

class Chit;

/* utility class to interpret a chit as a market. */
class MarketAI
{
public:
	MarketAI( Chit* chit );

	bool HasRanged( int au );
	bool HasMelee( int au );
	bool HasShield( int au );

	bool Has( int flag, int hardpoint, int au );

private:
	Chit* chit;
};

#endif // MARKET_AI_INCLUDED

