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

	bool IsEmpty() const;
	void MakeEmpty() {
		Wallet empty;
		*this = empty;
	}
	void Serialize( XStream* xs );

	int gold;
	int crystal[NUM_CRYSTAL_TYPES];
};

#endif

