#include "aineeds.h"
#include "../grinliz/glutil.h"

using namespace ai;
using namespace grinliz;

static const double DECAY_TIME = 100.0;	// in seconds

void Needs::DoTick( U32 delta, int flags )
{
	double dNeed = double(delta*1000) / DECAY_TIME;

	// Suspend needs if not at home team sectors.
	if ( !(flags & FLAG_AT_TEAM_SECTOR )) {
		return;
	}

	food	-= dNeed;
	social	-= dNeed;
	energy	-= dNeed;
	fun		-= dNeed;

	if ( flags & FLAG_IN_BATTLE ) {
		fun += dNeed * 5.0;
	}

	food	= Clamp( food,	 0.0, 1.0 );
	social	= Clamp( social, 0.0, 1.0 );
	energy	= Clamp( energy, 0.0, 1.0 );
	fun		= Clamp( fun,	 0.0, 1.0 );
}

