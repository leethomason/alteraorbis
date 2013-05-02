#ifndef RESERVE_BANK_INCLUDED
#define RESERVE_BANK_INCLUDED

#include "gamelimits.h"

class XStream;

class ReserveBank
{
public:
	ReserveBank();
	~ReserveBank();

	void Serialize( XStream* xs );

	int WithdrawDenizen();
	int WithdrawMonster();
	//int WithdrawBeastman();
	//int WithdrawVolcano( const SectorData& sd );

	static ReserveBank* Instance() { return instance; }

private:
	static ReserveBank* instance;

	int gold;	// can technically go zero, although the bank tries to avoid that
	int crystal[NUM_CRYSTAL_TYPES];
};

#endif // RESERVE_BANK_INCLUDED
