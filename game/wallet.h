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

struct Wallet
{
	Wallet() {
		gold = 0;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i) crystal[i] = 0; 
	}

	void Set(int g, int* c) {
		this->gold = g;
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			crystal[i] = c[i];
		}
	}

	// Only set true for the reserve bank.
	void SetCanBeUnderwater(bool under) { canGoUnderwater = under; }

	int Gold() const { return gold; }
	bool HasGold(int g) const { return gold >= g; }
	int Crystal(int i) const { GLASSERT(i >= 0 && i < NUM_CRYSTAL_TYPES); return crystal[i]; }
	const int* Crystal() const { return crystal; }

	void Deposit(Wallet* src, int g) {
		GLASSERT(src->gold >= g);
		gold += g;
		src->gold -= g;
	}

	void Deposit(Wallet* src, int g, const int* c) {
		Deposit(src, g);
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			GLASSERT(src->crystal[i] >= c[i]);
			crystal[i] += c[i];
			src->crystal[i] -= c[i];
		}
	}

	void Deposit(Wallet* src, const Wallet& w) {
		Deposit(src, w.Gold(), w.Crystal());
	}

	/*
	void AddGold( int g ) {
		this->gold += g;
		GLASSERT( this->gold >= 0 );
	}

	// Does the check before the increment.
	void AddCrystal( int id, int n=1 ) {
		if ( id < NUM_CRYSTAL_TYPES ) {
			crystal[id] += n;
			GLASSERT( crystal[id] >= 0 );
		}
	}

	void Add( const Wallet& w ) {
		this->gold += w.gold;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i )
			this->crystal[i] += w.crystal[i];
	}

	void Remove( const Wallet& w ) {
		this->gold -= w.gold;
		GLASSERT( this->gold >= 0 );
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
			this->crystal[i] -= w.crystal[i];
			GLASSERT( this->crystal[i] >= 0 );
		}
	}
	*/

	bool IsEmpty() const;

/*	Wallet EmptyWallet() {
		Wallet w = *this;
		Wallet empty;
		*this = empty;
		return w;
	}*/

	int NumCrystals() const {
		int count=0;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
			count += crystal[i];
		}
		return count;
	}

	void Serialize( XStream* xs );

	bool operator<=(const Wallet& rhs ) const {
		if ( gold <= rhs.gold ) {
			for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
				if ( crystal[i] > rhs.crystal[i] ) return false;
			}
			return true;
		}
		return false;
	}

	bool operator>(const Wallet& rhs ) const {
		return !(operator<=(rhs));
	}

private:
	bool canGoUnderwater;
	int gold;
	int crystal[NUM_CRYSTAL_TYPES];
};


/*
inline void Transfer(Wallet* to, Wallet* from, int gold) {
	GLASSERT(from->gold >= gold);
	from->gold -= gold;
	to->gold += gold;
}

inline void Transfer(Wallet* to, Wallet* from, const Wallet& w)
{
	// The amount to transfer is commonly the 'from' wallet.
	// So operate on 'to' before 'from'
	GLASSERT(from->gold >= w.gold);
	GLASSERT(to != &w);
	to->gold += w.gold;
	from->gold -= w.gold;

	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		GLASSERT(from->crystal[i] >= w.crystal[i]);
		to->crystal[i] += w.crystal[i];
		from->crystal[i] -= w.crystal[i];
	}
}

inline bool CanTransfer(const Wallet* to, const Wallet* from, int gold) {
	return(from->gold >= gold);
}


inline bool CanTransfer(const Wallet* to, const Wallet* from, const Wallet& w)
{
	// The amount to transfer is commonly the 'from' wallet.
	// So operate on 'to' before 'from'
	if (from->gold >= w.gold) {
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			if (from->crystal[i] < w.crystal[i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}
*/

#endif

