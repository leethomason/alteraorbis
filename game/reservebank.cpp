#include "reservebank.h"
#include "../xarchive/glstreamer.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

static const int GOLD_PER_DENIZEN  = 100;
static const int GOLD_PER_BEASTMAN =  20;
static const int GOLD_PER_MONSTER  =  10;
static const int ALL_GOLD =   TYPICAL_DENIZENS*GOLD_PER_DENIZEN 
							+ TYPICAL_BEASTMEN*GOLD_PER_BEASTMAN
							+ TYPICAL_MONSTERS*GOLD_PER_MONSTER;

ReserveBank* ReserveBank::instance = 0;

ReserveBank::ReserveBank()
{
	GLASSERT( instance == 0 );
	instance = this;

	// All the gold in the world.
	gold = ALL_GOLD;

	crystal[CRYSTAL_RED]		= TYPICAL_DOMAINS * 10;
	crystal[CRYSTAL_GREEN]		= TYPICAL_DOMAINS * 2;
	crystal[CRYSTAL_VIOLET]		= TYPICAL_DOMAINS / 2;
}


ReserveBank::~ReserveBank()
{
	GLASSERT( instance );
	instance = 0;
}


void ReserveBank::Serialize( XStream* xs )
{
	XarcOpen( xs, "ReserveBank" );
	XARC_SER( xs, gold );
	XARC_SER( xs, crystal[CRYSTAL_RED] );
	XARC_SER( xs, crystal[CRYSTAL_GREEN] );
	XARC_SER( xs, crystal[CRYSTAL_VIOLET] );
	XarcClose( xs );
}


int ReserveBank::WithdrawDenizen()
{
	int g = Min( GOLD_PER_DENIZEN, gold );
	gold -= g;
	return g;
}


int ReserveBank::WithdrawMonster()
{
	int g = Min( GOLD_PER_MONSTER, gold );
	gold -= g;
	return g;
}
