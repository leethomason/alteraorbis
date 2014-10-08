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
	wallet.SetCanBeUnderwater(true);

	// All the gold in the world.
	const int gold = ALL_GOLD;
	const int crystal[NUM_CRYSTAL_TYPES] = {
		ALL_CRYSTAL_GREEN,
		ALL_CRYSTAL_RED,
		ALL_CRYSTAL_BLUE,
		ALL_CRYSTAL_VIOLET
	};
	wallet.Set(gold, crystal);
}


ReserveBank::~ReserveBank()
{
	GLASSERT( instance );
	instance = 0;
}


void ReserveBank::Serialize( XStream* xs )
{
	XarcOpen( xs, "ReserveBank" );
	wallet.Serialize( xs );
	XarcClose( xs );
}


void ReserveBank::WithdrawDenizen(Wallet* dst)
{
	int crystal[NUM_CRYSTAL_TYPES] = { 0 };
	Withdraw(dst, GOLD_PER_DENIZEN, crystal);
}


void ReserveBank::WithdrawMonster(Wallet* dst, bool greater)
{
	int gold = greater ? GOLD_PER_GREATER : GOLD_PER_LESSER;
	int crystal[NUM_CRYSTAL_TYPES] = { 0 };

	static const float score[NUM_CRYSTAL_TYPES] = { 100, 50, 10, 1 };
	int pass = greater ? random.Rand(4) : random.Rand(2);

	for (int i = 0; i < pass; ++i) {
		int index = random.Select(score, NUM_CRYSTAL_TYPES);
		crystal[index] += 1;
	}
	Withdraw(dst, gold, crystal);
}


void ReserveBank::Withdraw(Wallet* dst, int gold, const int* crystal)
{
	dst->Deposit(&wallet, gold, crystal);
}


const int* ReserveBank::CrystalValue()
{
	if (crystalValue[0] == 0) {
		ForgeScript script(0, 2, 0);
		GameItem item;
		TransactAmt wallet;
		int tech = 0;
		script.Build(ForgeScript::GUN, ForgeScript::BLASTER, 0, 0, &item, &wallet, &tech, false);
		crystalValue[0] = int(item.GetValue() + 1.0f);

		crystalValue[CRYSTAL_RED] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_RED;
		crystalValue[CRYSTAL_BLUE] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_BLUE;
		crystalValue[CRYSTAL_VIOLET] = crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_VIOLET;
	}
	return crystalValue;
}


void ReserveBank::Buy(Wallet* src, const int* crystals)
{
	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		GLASSERT(src->Crystal(i) >= crystals[i]);
		int cost = CrystalValue(i) * crystals[i];
		src->Deposit(&wallet, cost);
	}
	wallet.Deposit(src, 0, crystals);
}
