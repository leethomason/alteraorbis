#include "team.h"
#include "../grinliz/glutil.h"
#include "../xegame/istringconst.h"

using namespace grinliz;


int GetTeam( const grinliz::IString& itemName )
{
	if ( itemName == IStringConst::arachnoid ) {
		return TEAM_RAT;
	}
	else if ( itemName == IStringConst::mantis ) {
		return TEAM_GREEN_MANTIS;
	}
	else if ( itemName == IStringConst::redMantis ) {
		return TEAM_RED_MANTIS;
	}
	else if (    itemName == IStringConst::cyclops
		      || itemName == IStringConst::fireCyclops
		      || itemName == IStringConst::shockCyclops )
	{
		return TEAM_CHAOS;
	}
	return TEAM_NEUTRAL;
}


int GetRelationship( int t0, int t1 )
{
	// t0 <= t1 to keep the logic simple.
	if ( t0 > t1 ) Swap( &t0, &t1 );

	if ( t1 == TEAM_CHAOS )   return RELATE_ENEMY;
	if ( t0 == TEAM_NEUTRAL ) return RELATE_NEUTRAL;
	if ( t0 == t1 ) return RELATE_FRIEND;

	if ( t0 == TEAM_VISITOR ) {
		if ( t1 == TEAM_HOUSE0 )
			return RELATE_FRIEND;
		else
			return RELATE_ENEMY;
	}
	return RELATE_ENEMY;
}
