#include "team.h"
#include "../grinliz/glutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/chit.h"
#include "../xegame/itemcomponent.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

int Team::idPool = 0;

grinliz::IString Team::TeamName( int team )
{
	IString name;
	CStr<64> str;
	int group = 0, id = 0;
	SplitID(team, &group, &id);

	switch ( group ) {
	case TEAM_HOUSE:
		str.Format("House-%x", id);
		name = StringPool::Intern( str.c_str() );	
		break;

	default:
		break;
	}

	return name;
}


int Team::GetTeam( const grinliz::IString& itemName )
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
	GLASSERT(0);
	return TEAM_NEUTRAL;
}


int Team::GetRelationship( int _t0, int _t1 )
{
	int t0 = 0, t1 = 0;
	SplitID(_t0, &t0, 0);
	SplitID(_t1, &t1, 0);

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
		if ( t1 == TEAM_HOUSE )
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


int Team::GetRelationship( Chit* chit0, Chit* chit1 )
{
	if ( chit0->GetItem() && chit1->GetItem() ) {
		return GetRelationship( chit0->GetItem()->team,
								chit1->GetItem()->team );
	}
	return RELATE_NEUTRAL;
}


void Team::Serialize(XStream* xs)
{
	XarcOpen(xs,"Team");
	XARC_SER(xs, idPool);
	XarcClose(xs);
}
