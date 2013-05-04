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


bool Wallet::IsEmpty() const {
	if ( gold == 0 ) {
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
			if ( crystal[i] )
				return false;
		}
		return true;
	}
	return false;
}


void Wallet::Serialize( XStream* xs )
{
	XarcOpen( xs, "Wallet" );
	XARC_SER( xs, gold );
	XARC_SER( xs, crystal[CRYSTAL_RED] );
	XARC_SER( xs, crystal[CRYSTAL_GREEN] );
	XARC_SER( xs, crystal[CRYSTAL_VIOLET] );
	XarcClose( xs );
}


ReserveBank::ReserveBank()
{
	GLASSERT( instance == 0 );
	instance = this;

	// All the gold in the world.
	bank.gold = ALL_GOLD;

	bank.crystal[CRYSTAL_RED]		= TYPICAL_DOMAINS * 10;
	bank.crystal[CRYSTAL_GREEN]		= TYPICAL_DOMAINS * 2;
	bank.crystal[CRYSTAL_VIOLET]		= TYPICAL_DOMAINS / 2;
}


ReserveBank::~ReserveBank()
{
	GLASSERT( instance );
	instance = 0;
}


void ReserveBank::Serialize( XStream* xs )
{
	XarcOpen( xs, "ReserveBank" );
	bank.Serialize( xs );
	XarcClose( xs );
}


int ReserveBank::WithdrawDenizen()
{
	int g = Min( GOLD_PER_DENIZEN, bank.gold );
	bank.gold -= g;
	return g;
}


Wallet ReserveBank::WithdrawMonster()
{
	Wallet w;
	w.gold = WithdrawGold( GOLD_PER_MONSTER );
	if ( random.Rand(10) == 0 ) {
		w.AddCrystal( WithdrawRandomCrystal() );
	}
	return w;
}


int ReserveBank::WithdrawVolcanoGold()
{
	int g = Min( (int)random.Rand( GOLD_PER_MONSTER), bank.gold );
	bank.gold -= g;
	return g;
}


int ReserveBank::WithdrawRandomCrystal()
{
	int r = random.Rand( 100 );
	int c = NO_CRYSTAL;
	if ( r == 0 )		c = CRYSTAL_VIOLET;
	else if ( r < 8 )	c = CRYSTAL_GREEN;
	else				c = CRYSTAL_RED;

	if ( c != NO_CRYSTAL && bank.crystal[c] ) {
		bank.crystal[c] -= 1;
		return c;
	}
	return NO_CRYSTAL;
}


Wallet ReserveBank::WithdrawVolcano()
{
	Wallet w;
	w.gold		= WithdrawVolcanoGold();
	w.AddCrystal( WithdrawRandomCrystal() );
	return w;
}

