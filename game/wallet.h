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

class XStream;

struct Wallet
{
	Wallet() {
		gold = 0;
		for( int i=0; i<NUM_CRYSTAL_TYPES; ++i) crystal[i] = 0; 
	}
	// Does the check before the increment.
	void AddCrystal( int id ) {
		if ( id < NUM_CRYSTAL_TYPES ) crystal[id] += 1;
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

	bool IsEmpty() const;
	void MakeEmpty() {
		Wallet empty;
		*this = empty;
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

	int gold;
	int crystal[NUM_CRYSTAL_TYPES];
};

#endif

