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
class CoreScript;
class Web;

enum {
	TEAM_NEUTRAL,	// neutral to all teams. Does NOT have an id: value should be truly 0.
	TEAM_CHAOS,
	
	TEAM_RAT,	
	TEAM_GREEN_MANTIS,
	TEAM_RED_MANTIS,
	TEAM_TROLL,

	TEAM_HOUSE,		// denizen houses (primarily human)
	TEAM_GOB,
	TEAM_KAMAKIRI,
	TEAM_VISITOR,
	TEAM_DEITY,
	
	NUM_TEAMS
};

enum {
	DEITY_MOTHER_CORE = 1
};

enum {
	TEAM_ID_LEFT  = 0x1fffff,
	TEAM_ID_RIGHT = 0x2fffff,
};


enum class ERelate {
	FRIEND,
	NEUTRAL,
	ENEMY
};


class Team
{
public:
	Team();
	~Team();

	static Team* Instance() { return instance; }

	void Serialize(XStream* xs);
	void DoTick(int delta);

	// Team name, where it has one.
	static grinliz::IString TeamName(int team);
	// Given a MOB name, return the team.
	static int GetTeam(const grinliz::IString& name);

	// Take a team, and add an id to it.
	int GenTeam(int teamGroup) {
		teamGroup = Group(teamGroup);
		int team = teamGroup | ((++idPool) << 8);
		return team;
	}

	// A base relationship is symmetric (both parties feel the same way)
	// and based on species.
	static ERelate BaseRelationship(int team0, int team1);

	// The current relationship is symmetric
	ERelate GetRelationship(int team0, int team1);
	ERelate GetRelationship(Chit* chit0, Chit* chit1);

	// The attitude is asymetric 
	int CalcAttitude(CoreScript* center, CoreScript* eval, const Web* web);
	int Attitude(CoreScript* center, CoreScript* eval);

	// Go to war? If 'commit' is true, actually do it,
	// else a query. Returns success.
	bool War(CoreScript* c0, CoreScript* c1, bool commit, const Web* web);
	int  Peace(CoreScript* c0, CoreScript* c1, bool commit, const Web* web);

	static void SplitID(int t, int* group, int* id)	{
		if (group)
			*group = (t & 0xff);
		if (id)
			*id = ((t & 0xffffff00) >> 8);
	}

	static int CombineID(int group, int id) {
		GLASSERT(group >= 0 && group < 256);
		GLASSERT(id >= 0);
 		return group | (id << 8);
	}

	static int Group(int team) {
		return team & 0xff;
	}

	static int ID(int team) {
		return ((team & 0xffffff00) >> 8);
	}

	static bool IsRogue(int team) {
		return (team & 0xffffff00) == 0;
	}

	static bool IsDeityCore(int team) {
		int group = Group(team);
		return (group == TEAM_TROLL) || (group == TEAM_DEITY);
	}

	static bool IsDefault(const grinliz::IString& name, int team);

private:
	enum {
		TREATY_TIME = 10*60*1000	// minutes of game time.
	};

	static ERelate AttitudeToRelationship(int d) {
		if (d > 0) return ERelate::FRIEND;
		if (d < 0) return ERelate::ENEMY;
		return ERelate::NEUTRAL;
	}
	static int RelationshipToAttitude(ERelate r) {
		if (r == ERelate::FRIEND) return 2;
		if (r == ERelate::ENEMY) return -1;
		return 0;
	}

	int idPool;

	struct TeamKey {
		TeamKey() : t0(0), t1(0) {}

		TeamKey(int origin, int stanceTo) {
			this->t0 = origin;
			this->t1 = stanceTo;
		}

		static bool Equal(const TeamKey& a, const TeamKey& b) {
			return a.t0 == b.t0 && a.t1 == b.t1;
		}
		static bool Less(const TeamKey& a, const TeamKey& b) {
			if (a.t0 < b.t0) return true;
			else if (a.t0 > b.t0) return false;
			else return a.t1 < b.t1;
		}
		static U32 Hash(const TeamKey& t) {
			return t.t0 + t.t1;
		}

		int T0() const { return t0; }
		int T1() const { return t1; }

	private:
		int t0, t1;
	};

	grinliz::HashTable<TeamKey, int, TeamKey> hashTable;

	struct SymmetricTK {
		SymmetricTK() : t0(0), t1(0), warTimer(0), peaceTimer(0) {}
		SymmetricTK(int _t0, int _t1) {
			t0 = grinliz::Min(_t0, _t1);
			t1 = grinliz::Max(_t0, _t1);
			warTimer = peaceTimer = 0;
		}

		int t0;
		int t1;
		int warTimer;
		int peaceTimer;

		bool operator==(const SymmetricTK& rhs) {
			return t0 == rhs.t0 && t1 == rhs.t1;
		}
		void Serialize(XStream* xs);
	};

	grinliz::CDynArray<SymmetricTK> treaties;

	static Team* instance;
};


#endif // LUMOS_TEAM_INCLUDED
