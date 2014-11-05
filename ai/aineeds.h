#ifndef AI_NEEDS_INCLUDED
#define AI_NEEDS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glutil.h"

class XStream;
class Personality;

namespace ai {

/*
	Basic question: which way do these go???
	Is hunger=0 really hungry?
	The Sims uses the bar is filled to denote
	the need is met; we'll use that standard.

	So food=1 is doing great,
	food=0 is starving.

	- food (elixir,plants)
	- energy (sleep tube)
	- fun (battle, bar, club, work, adventuring, crafting)

	Removed social, made part of 'fun'. (It's fun to talk to other
	people if you are extroverted.)

	HAS_NEEDS and USES_BUILDINGS aren't quite orthogonal.

	HAS_NEEDS, USES_BUILDINGS (denizen)
		wandering: morale reduces slowly.	morale==SMALL goes home
		-- avoiding for more code: arrive home: morale increased to 1/2
		at friendly: morale/needs consumed. morale==SMALL goes home
		at home: morale/needs consumed. morale==0 goes bad

	!HAS_NEEDS, USES_BUILDINGS (troll)
		at friendly/home: needs reduce, and buildings provide needs,
		but morale isn't changed by needs

	HAS_NEEDS, !USES_BUILDINGS
		senseless at this time.
*/
class Needs
{
public:
	Needs();

	enum {
		FOOD,
		ENERGY,
		FUN,
		NUM_NEEDS
	};

	static const char* Name( int i );
	double Value( int i ) const { GLASSERT( i >= 0 && i < NUM_NEEDS ); return need[i]; }

	void SetMorale( double v ) { morale = v; }
	void AddMorale( double v ) { morale += v; morale = grinliz::Clamp( morale, 0.0, 1.0 ); }
	double Morale() const { return morale; }

	void Set( int i, double v ) { GLASSERT( i >= 0 && i < NUM_NEEDS ); need[i] = v; ClampNeeds(); }
	void Add( const Needs& needs, double scale );
	void Add( int i, double v ) { GLASSERT( i >= 0 && i < NUM_NEEDS ); need[i] += v; ClampNeeds(); }

	void SetZero() { for( int i=0; i<NUM_NEEDS; ++i ) need[i] = 0; }
	void SetFull() { for( int i=0; i<NUM_NEEDS; ++i ) need[i] = 1; }
	bool IsZero() const { 
		for( int i=0; i<NUM_NEEDS; ++i ) {
			if ( need[i] > 0 ) 
				return false;
		}
		return true;
	}

	// Intended to be pretty long - every second or so.
	void DoTick( U32 delta, bool inBattle, bool lowerDifficulty, const Personality* );
	// A morale-only tick, generally if a mob (with HAS_NEEDS) is travelling.
	void DoTravelTick(U32 delta);

	// Needs to load from XML - declared in the itemdef.xml
	void Serialize( XStream* xs );

	static double DecayTime();

private:
	void ClampNeeds();
	double need[NUM_NEEDS];
	double morale;
};


};

#endif // AI_NEEDS_INCLUDED
