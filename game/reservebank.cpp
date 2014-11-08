#include "reservebank.h"
#include "../xarchive/glstreamer.h"
#include "../grinliz/glstringutil.h"
#include "../script/itemscript.h"
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
	TransactAmt amt;
	amt.AddGold(GOLD_PER_DENIZEN);

	// If bank is low, don't give new spawns money.
	if (wallet.CanWithdraw(amt)) {
		dst->Deposit(&wallet, amt);
	}
}


void ReserveBank::WithdrawMonster(Wallet* dst, bool greater)
{
	TransactAmt amt;
	int gold = greater ? GOLD_PER_GREATER : GOLD_PER_LESSER;
	amt.AddGold(gold);

	static const float score[NUM_CRYSTAL_TYPES] = { 100, 50, 10, 1 };
	int pass = greater ? random.Rand(4) : random.Rand(2);

	for (int i = 0; i < pass; ++i) {
		int index = random.Select(score, NUM_CRYSTAL_TYPES);
		amt.AddCrystal(index, 1);
	}
	// If bank is low, don't give new spawns money.
	if (wallet.CanWithdraw(amt)) {
		dst->Deposit(&wallet, amt);
	}
}


void ReserveBank::Withdraw(Wallet* dst, int gold, const int* crystal)
{
	dst->Deposit(&wallet, gold, crystal);
}


const int* ReserveBank::CrystalValue()
{
	if (crystalValue[0] == 0) {
		TransactAmt wallet;
		int tech = 0;
		GameItem* item = ItemDefDB::Instance()->Get("blaster").Clone();
		crystalValue[0] = int(item->GetValue() + 1.0f);
		delete item; item = 0;

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
