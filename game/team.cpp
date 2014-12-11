#include "team.h"
#include "../grinliz/glutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/chit.h"
#include "../xegame/itemcomponent.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

int Team::idPool = 1;	// id=0 is rogue.

grinliz::IString Team::TeamName(int team)
{
	IString name;
	CStr<64> str;
	int group = 0, id = 0;
	SplitID(team, &group, &id);

	switch (group) {
		case TEAM_HOUSE:
		if (id)
			str.Format("House-%x", id);
		else
			str = "House";
		name = StringPool::Intern(str.c_str());
		break;

		case TEAM_TROLL:
		// Since Trolls can't build anything,
		// any troll core is by definition
		// Truulga. (At least at this point.)
		name = ISC::Truulga;
		break;

		case TEAM_GOB:
		if (id)
			str.Format("Gobmen-%x", id);
		else
			str = "Gobmen";
		name = StringPool::Intern(str.c_str());
		break;

		case TEAM_KAMAKIRI:
		if (id)
			str.Format("Kamakiri-%x", id);
		else
			str = "Kamakiri";
		name = StringPool::Intern(str.c_str());
		break;

		case TEAM_DEITY:
		name = ISC::deity;
		break;

		default:
		break;
	}

	return name;
}


int Team::GetTeam( const grinliz::IString& itemName )
{
	if (itemName == ISC::trilobyte) {
		return TEAM_RAT;
	}
	else if ( itemName == ISC::mantis ) {
		return TEAM_GREEN_MANTIS;
	}
	else if ( itemName == ISC::redMantis ) {
		return TEAM_RED_MANTIS;
	}
	else if ( itemName == ISC::troll ) {
		return TEAM_TROLL;
	}
	else if (itemName == ISC::gobman) {
		return TEAM_GOB;
	}
	else if (itemName == ISC::kamakiri) {
		return TEAM_KAMAKIRI;
	}
	else if (itemName == ISC::deity) {
		return TEAM_DEITY;
	}
	else if (    itemName == ISC::cyclops
		      || itemName == ISC::fireCyclops
		      || itemName == ISC::shockCyclops )
	{
		return TEAM_CHAOS;
	}
	else if (    itemName == ISC::human
			  || itemName == ISC::humanFemale
			  || itemName == ISC::humanMale) 
	{
		return TEAM_HOUSE;
	}
	GLASSERT(0);
	return TEAM_NEUTRAL;
}


bool Team::IsDefault(const IString& str, int team)
{
	if (team == TEAM_NEUTRAL || team == TEAM_CHAOS) return true;
	return GetTeam(str) == Group(team);
}

int Team::GetRelationship( int _t0, int _t1 )
{
	int t0 = 0, t1 = 0;
	int g0 = 0, g1  =0 ;
	SplitID(_t0, &t0, &g0);
	SplitID(_t1, &t1, &g1);

	// t0 <= t1 to keep the logic simple.
	if ( t0 > t1 ) Swap( &t0, &t1 );

	// CHAOS hates all - even each other.
	if ( t0 == TEAM_CHAOS || t1 == TEAM_CHAOS)
		return RELATE_ENEMY;
	// Likewise, neutral is neutral even to themselves.
	if (t0 == TEAM_NEUTRAL || t1 == TEAM_NEUTRAL)
		return RELATE_NEUTRAL;

	GLASSERT(t0 >= 0 && t0 < NUM_TEAMS);
	GLASSERT(t1 >= 0 && t1 < NUM_TEAMS);

	static const int F = RELATE_FRIEND;
	static const int E = RELATE_ENEMY;
	static const int N = RELATE_NEUTRAL;
	static const int OFFSET = TEAM_RAT;
	static const int NUM = NUM_TEAMS - OFFSET;

	static const int relate[NUM][NUM] = {
		{ F, E, E, E, E, E, E, E, N },		// rat
		{ 0, F, E, N, E, E, F, N, N },		// green
		{ 0, 0, F, N, E, E, E, E, N },		// red
		{ 0, 0, 0, F, E, N, N, N, N },		// troll 
		{ 0, 0, 0, 0, F, N, E, F, N },		// house
		{ 0, 0, 0, 0, 0, F, E, F, N },		// gobmen
		{ 0, 0, 0, 0, 0, 0, F, N, N },		// kamakiri
		{ 0, 0, 0, 0, 0, 0, 0, F, F },		// visitor
		{ 0, 0, 0, 0, 0, 0, 0, 0, N },		// deity
	};
	GLASSERT(t0 - OFFSET >= 0 && t0 - OFFSET < NUM);
	GLASSERT(t1 - OFFSET >= 0 && t1 - OFFSET < NUM);
	GLASSERT(t1 >= t0);

	// Special handling for left/right battle scene matchups:
	if (   t0 == t1 
		&& ((g0 == TEAM_ID_LEFT && g1 == TEAM_ID_RIGHT) || (g0 == TEAM_ID_RIGHT && g1 == TEAM_ID_LEFT))) 
	{
		return RELATE_ENEMY;
	}

	return relate[t0-OFFSET][t1-OFFSET];
}


int Team::GetRelationship( Chit* chit0, Chit* chit1 )
{
	if ( chit0->GetItem() && chit1->GetItem() ) {
		return GetRelationship( chit0->GetItem()->Team(),
								chit1->GetItem()->Team() );
	}
	return RELATE_NEUTRAL;
}


void Team::Serialize(XStream* xs)
{
	XarcOpen(xs,"Team");
	XARC_SER(xs, idPool);
	XarcClose(xs);
}
