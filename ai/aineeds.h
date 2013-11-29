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
	Needs() : food(1), social(1), energy(1), fun(1) {}

	double	food,
			social,
			energy,
			fun;

	// Intended to be pretty long - every second or so.

	enum {
		FLAG_IN_BATTLE,
		FLAG_AT_TEAM_SECTOR
	};

	void DoTick( U32 delta, int flags );
};


};

#endif // AI_NEEDS_INCLUDED
