#ifndef AI_NEEDS_INCLUDED
#define AI_NEEDS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

namespace ai {

/*
	Basic question: which way do these go???
	Is hunger=0 really hungry?
	The Sims uses the bar is filled to denote
	the need is met; we'll use that standard.

	So food=1 is doing great,
	food=0 is starving.

	- food (elixir,plants)
	- social (bar, club, squad)
	- energy (sleep tube)
	- fun (battle, bar, club, work, adventuring, crafting)
*/
class Needs
{
public:
	Needs()  { for( int i=0; i<NUM_NEEDS; ++i ) need[i] = 1; }

	enum {
		FOOD,
		SOCIAL,
		ENERGY,
		FUN,
		NUM_NEEDS
	};

	static const char* Name( int i );
	double Value( int i ) const { GLASSERT( i >= 0 && i < NUM_NEEDS ); return need[i]; }

	// Intended to be pretty long - every second or so.
	void DoTick( U32 delta, bool inBattle );

private:

	double	need[NUM_NEEDS];
};


};

#endif // AI_NEEDS_INCLUDED
