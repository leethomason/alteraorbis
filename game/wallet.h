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

#ifndef WALLET_INCLUDED
#define WALLET_INCLUDED

#include "gamelimits.h"

class XStream;

class Wallet
{
public:
	Wallet() {
		canGoUnderwater = false;
		closed = false;
		gold = 0;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i) crystal[i] = 0; 
	}
	
	// Only to be called by the reserve bank.
	void Set(int g, const int* c) {
		this->gold = g;
		if (c) {
			for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
				crystal[i] = c[i];
			}
		}
	}

	// Only set true for the reserve bank.
	// NOT serialized.
	void SetCanBeUnderwater(bool under) { canGoUnderwater = under; }

	int Gold() const				{ return gold; }
	bool HasGold(int g) const		{ return gold >= g; }
	int Crystal(int i) const		{ GLASSERT(i >= 0 && i < NUM_CRYSTAL_TYPES); return crystal[i]; }
	const int* Crystal() const		{ return crystal; }

	// Move money from one wallet to another. The only way
	// to create money is the Set() method. With that,
	// we don't need a Withdraw() function since money is 
	// always being moved from one Wallet to another.
	void Deposit(Wallet* src, int g) {
		GLASSERT(!closed);
		GLASSERT(g >= 0);	// the checks for 'enough gold' aren't correct if negative.
		GLASSERT(src->canGoUnderwater || src->gold >= g);
		gold += g;
		src->gold -= g;
	}

	void Deposit(Wallet* src, int g, const int* c) {
		Deposit(src, g);
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			GLASSERT(c[i] >= 0);
			GLASSERT(src->canGoUnderwater || src->crystal[i] >= c[i]);
			crystal[i] += c[i];
			src->crystal[i] -= c[i];
		}
	}

	// the amount can be the src or target.
	void Deposit(Wallet* src, const Wallet& amount) {
		Deposit(src, amount.Gold(), amount.Crystal());
	}

	void DepositAll(Wallet* src) {
		Deposit(src, src->Gold(), src->Crystal());
	}

	bool IsEmpty() const;

	int NumCrystals() const {
		int count=0;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
			count += crystal[i];
		}
		return count;
	}

	void Serialize( XStream* xs );

	bool CanWithdraw(const Wallet& amt) const {
		if (amt.Gold() > Gold()) return false;

		for (int i = 0; i < NUM_CRYSTAL_TYPES; i++) {
			if (amt.Crystal(i) > Crystal(i)) {
				return false;
			}
		}
		return true;
	}

	// Debugging; this wallet is no longer accepting deposits.
	void SetClosed() { closed = true; }

private:
	Wallet(const Wallet& w);	// private, unimplemented.

protected:
	bool canGoUnderwater;
	bool closed;
	int gold;
	int crystal[NUM_CRYSTAL_TYPES];
};


class TransactAmt : public Wallet
{
public:
	void Clear() {
		gold = 0;
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			crystal[i] = 0;
		}
	}

	void Add(const Wallet& w) {
		gold += w.Gold();
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			crystal[i] += w.Crystal(i);
		}
	}

	void AddCrystal(int type, int count) {
		GLASSERT(type >= 0 && type < NUM_CRYSTAL_TYPES);
		crystal[type] += count;
	}

	void AddGold(int g) {
		gold += g;
	}
};

#endif

