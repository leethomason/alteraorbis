/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

	Wallet wallet;

	void Serialize( XStream* xs );

	int WithdrawDenizen();
	Wallet WithdrawMonster();

	const int* CrystalValue() const;

	// There is no CanBuy(), because having the economy
	// falter because the bank is out of money is 
	// crazy. Buy() still works. However, Withdraw()
	// and other functions will get cut way back if
	// the bank is under water.
	void Buy(Wallet* src, const int* crystals);

	// Withdraws 1 or 0 crystals. type is returned.
	int WithdrawRandomCrystal();

	static ReserveBank* Instance() { return instance; }

private:
	static ReserveBank* instance;
	grinliz::Random random;
	int crystalValue[NUM_CRYSTAL_TYPES];
};

#endif // RESERVE_BANK_INCLUDED
