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

#ifndef LUMOS_TEAM_INCLUDED
#define LUMOS_TEAM_INCLUDED

#include "../grinliz/glstringutil.h"

class Chit;
class XStream;

enum {
	TEAM_NEUTRAL,	// neutral to all teams. Does NOT have an id: value should be truly 0.
	TEAM_CHAOS,
	
	TEAM_RAT,	
	TEAM_GREEN_MANTIS,
	TEAM_RED_MANTIS,
	TEAM_TROLL,

	TEAM_HOUSE,		// denizen houses (primarily human)
	TEAM_GOB,
	TEAM_VISITOR,
	
	NUM_TEAMS
};


enum {
	RELATE_FRIEND,
	RELATE_ENEMY,
	RELATE_NEUTRAL
};


class Team
{
public:
	static void Serialize(XStream* xs);

	// Team name, where it has one.
	static grinliz::IString TeamName(int team);
	// Given a MOB name, return the team.
	static int GetTeam(const grinliz::IString& name);

	// Take a team, and add an id to it.
	static int GenTeam(int teamGroup) {
		int team = teamGroup | ((++idPool) << 8);
		return team;
	}

	static int GetRelationship(int team0, int team1);
	static int GetRelationship(Chit* chit0, Chit* chit1);

	static void SplitID(int t, int* group, int* id)	{
		if (group)
			*group = (t & 0xff);
		if (id)
			*id = ((t & 0xffffff00) >> 8);
	}

private:
	static int idPool;
};

inline int TeamGroup(int team) {
	return team & 0xff;
}

#endif // LUMOS_TEAM_INCLUDED
