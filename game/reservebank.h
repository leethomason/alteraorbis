#ifndef RESERVE_BANK_INCLUDED
#define RESERVE_BANK_INCLUDED

#include "gamelimits.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glutil.h"
#include "wallet.h"

class XStream;


class ReserveBank
{
public:
	ReserveBank();
	~ReserveBank();

	void Serialize( XStream* xs );

	int Gold() const					{ return bank.gold; }
	const Wallet& GetWallet() const		{ return bank; }

	int WithdrawDenizen();
	Wallet WithdrawMonster();

	int WithdrawVolcanoGold();
	Wallet WithdrawVolcano();

	// Withdraws 1 or 0 crystals. type is returned.
	int WithdrawRandomCrystal();
	int WithdrawGold( int g ) { g = grinliz::Min( g, bank.gold ); bank.gold -= g; return g; }

	static ReserveBank* Instance() { return instance; }

private:
	static ReserveBank* instance;
	grinliz::Random random;

	Wallet bank;
};

#endif // RESERVE_BANK_INCLUDED
