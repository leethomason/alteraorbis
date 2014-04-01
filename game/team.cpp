#include "team.h"
#include "../grinliz/glutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/chit.h"
#include "../xegame/itemcomponent.h"

using namespace grinliz;


grinliz::IString TeamName( int team )
{
	IString name;
	switch ( team ) {
	case TEAM_HOUSE0:	name = StringPool::Intern( "House0" );	break;

	default:
		break;
	}

	return name;
}


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
	else if ( itemName == IStringConst::troll ) {
		return TEAM_TROLL;
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

	// Likewise, neutral is neutral even to themselves.
	if (t0 == TEAM_NEUTRAL)
		return RELATE_NEUTRAL;
	// CHAOS hates all - even each other.
	if ( t1 == TEAM_CHAOS )   
		return RELATE_ENEMY;
	// Exceptions handled: everyone is friends with themselves.
	if ( t0 == t1 ) 
		return RELATE_FRIEND;

	// Everyone hates RAT
	if ( t1 == TEAM_RAT )
		return RELATE_ENEMY;

	if ( t0 == TEAM_VISITOR ) {
		if ( t1 == TEAM_HOUSE0 )
			return RELATE_FRIEND;
		else
			return RELATE_ENEMY;
	}

	// Trolls are neutral to the mantis, so they can disperse out.
	if ( t1 == TEAM_TROLL ) {
		if ( t0 == TEAM_GREEN_MANTIS || t0 == TEAM_RED_MANTIS )
			return RELATE_NEUTRAL;
	}

	return RELATE_ENEMY;
}


int GetRelationship( Chit* chit0, Chit* chit1 )
{
	if ( chit0->GetItem() && chit1->GetItem() ) {
		return GetRelationship( chit0->GetItem()->primaryTeam,
								chit1->GetItem()->primaryTeam );
	}
	return RELATE_NEUTRAL;
}
