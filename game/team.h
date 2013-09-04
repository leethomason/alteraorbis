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

enum {
	TEAM_NEUTRAL,	// neutral to all teams.
	TEAM_VISITOR,
	
	TEAM_RAT,	
	TEAM_GREEN_MANTIS,
	TEAM_RED_MANTIS,

	TEAM_HOUSE0,

	TEAM_CHAOS
};


int GetTeam( const grinliz::IString& itemName );

enum {
	RELATE_FRIEND,
	RELATE_ENEMY,
	RELATE_NEUTRAL
};

int GetRelationship( int team0, int team1 );
int GetRelationship( Chit* chit0, Chit* chit1 );

#endif // LUMOS_TEAM_INCLUDED
