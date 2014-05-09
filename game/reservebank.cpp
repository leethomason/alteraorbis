#include "reservebank.h"
#include "../xarchive/glstreamer.h"
#include "../grinliz/glstringutil.h"
#include "../script/forgescript.h"
#include "../game/gameitem.h"

using namespace grinliz;

ReserveBank* ReserveBank::instance = 0;


bool Wallet::IsEmpty() const {
	int count = gold;
	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		count += crystal[i];
	}
	return count == 0;
}


void Wallet::Serialize( XStream* xs )
{
	XarcOpen( xs, "Wallet" );
	if (xs->Loading() || (xs->Saving() && !this->IsEmpty())) {
		XARC_SER(xs, gold);
		XARC_SER(xs, crystal[CRYSTAL_GREEN]);
		XARC_SER(xs, crystal[CRYSTAL_RED]);
		XARC_SER(xs, crystal[CRYSTAL_BLUE]);
		XARC_SER(xs, crystal[CRYSTAL_VIOLET]);
	}
	XarcClose( xs );
}


ReserveBank::ReserveBank()
{
	GLASSERT( instance == 0 );
	instance = this;
	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		crystalValue[i] = 0;
	}

	// All the gold in the world.
	bank.gold = ALL_GOLD;

	bank.crystal[CRYSTAL_GREEN]		= ALL_CRYSTAL_GREEN;
	bank.crystal[CRYSTAL_RED]		= ALL_CRYSTAL_RED;
	bank.crystal[CRYSTAL_BLUE]		= ALL_CRYSTAL_BLUE;
	bank.crystal[CRYSTAL_VIOLET]	= ALL_CRYSTAL_VIOLET;
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
	if ( bank.gold >= GOLD_PER_MONSTER ) {
		w.AddGold( GOLD_PER_MONSTER );
		bank.AddGold( -GOLD_PER_MONSTER );
	}
	if ( random.Rand(2) == 0 ) {
		w.AddCrystal( WithdrawRandomCrystal() );
	}
	return w;
}


int ReserveBank::WithdrawRandomCrystal()
{
	static const float score[NUM_CRYSTAL_TYPES] = { 100,50,10,1 };
	int type = random.Select( score, NUM_CRYSTAL_TYPES );

	if ( bank.crystal[type] ) {
		bank.crystal[type] -= 1;
		return type;
	}
	return NO_CRYSTAL;
}


const int* ReserveBank::CrystalValue()
{
	if (crystalValue[0] == 0) {
		ForgeScript script(0, 2, 0);
		GameItem item;
		Wallet wallet;
		int tech = 0;
		script.Build(ForgeScript::GUN, ForgeScript::BLASTER, 0, 0, &item, &wallet, &tech, false);
		crystalValue[0] = int(item.GetValue() + 1.0f);

		crystalValue[CRYSTAL_RED] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_RED;
		crystalValue[CRYSTAL_BLUE] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_BLUE;
		crystalValue[CRYSTAL_VIOLET] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_VIOLET;
	}
	return crystalValue;
}
