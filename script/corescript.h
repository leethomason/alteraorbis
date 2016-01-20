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

#ifndef CORESCRIPT_INCLUDED
#define CORESCRIPT_INCLUDED

#include "../xegame/cticker.h"
#include "../game/gamelimits.h"
#include "../xegame/component.h"
#include "../xegame/chitbag.h"

#define SPAWN_MOBS 1

class WorldMap;
class WorkQueue;
class LumosChitBag;
class CoreScript;
class Model;
class ChitContext;

#define CCoreArray grinliz::CArray<CoreScript*, 32 >

struct CoreAchievement
{
	void Clear() { techLevel = 0; gold = 0; population = 0; civTechScore = 0; }
	void Serialize(XStream* xs);

	static const char* CivTechDescription(int score);

	int			techLevel;
	int			gold;
	int			population;
	int			civTechScore;
};


class CoreScript : public Component
{
	typedef Component super;
public:
	CoreScript();
	virtual ~CoreScript();

	virtual void Serialize( XStream* xs );
	virtual void OnAdd(Chit* chit, bool init);
	virtual void OnRemove();

	virtual int DoTick( U32 delta );
	virtual const char* Name() const	{ return "CoreScript"; }
	virtual CoreScript* ToCoreScript()	{ return this; }

	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);

	int GetPave();

	void AddCitizen(Chit* chit);
	bool IsCitizen(Chit* chit);
	bool IsCitizen(int id);
	bool IsCitizenItemID(int id);
	int  Citizens(CChitArray* arr);

	// 0-3, -1 if not in squad
	int SquadID(int chitID);
	int Squaddies(int squadID, CChitArray* arr);
	bool IsSquaddieOnMission(int chitID, int* squadID);
	bool ChangeSquad(Chit* chit, int targetSquad);
	bool WantsToConquer(const grinliz::Vector2I& sector);

	static int MaxCitizens(int nTemples);
	int MaxCitizens();
	int NumTemples();

	int CorePower();	// approximation of the military strength of the core
	int CoreWealth();	// amount of money in the core's treasury
	float CivTech();	// current civtech score, from 0-1

	/*	Controls flags are simple "denizen attractors"
		in the same domain. When a denizen arrives
		the flag is cleared. They are assigned to the
		first available denizen.
	*/
	void AddFlag(const grinliz::Vector2I& pos);
	void RemoveFlag(const grinliz::Vector2I& pos);
	void ToggleFlag(const grinliz::Vector2I& pos);
	// Get a flag in the CoreScript's sector that is
	// not yet in the list.
	grinliz::Vector2I GetAvailableFlag();

	/*	Waypoints are destinations that may involve
		grid travel. Waypoints are assigned to 
		a particular squad, and movement 
		can be coordinated.
	*/
	void				SetWaypoints(int squadID, const grinliz::Vector2I& dest);
	grinliz::Vector2I	GetWaypoint(int squadID);	// gets the current waypoint
	grinliz::Vector2I	GetLastWaypoint(int squadID);
	const grinliz::CDynArray<grinliz::Vector2I>& GetWaypoints(int squadID) const {
		GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
		return waypoints[squadID];
	}
	void PopWaypoint(int squadID);

	bool InUse() const;

	WorkQueue* GetWorkQueue()		{ return workQueue; }
	const CoreAchievement& GetAchievement() const { return achievement; }

	// Current tech:
	float GetTech() const			{ return float(tech); }
	// The number of temples+1:
	int   MaxTech();
	void  AddTech();

	// Each task pushes a position for that task,
	// and removes it when done/cancelled. (Some careful
	// code there.) May be a simpler way to see if
	// someone is assigned to a task at location. However,
	// approximate, since nothing but the location is tracked.
	void AddTask(const grinliz::Vector2I& pos2i);
	void RemoveTask(const grinliz::Vector2I& pos2i);
	bool HasTask(const grinliz::Vector2I& pos2i);

	static CoreScript* GetCore(const grinliz::Vector2I& sector) { 
		GLASSERT(sector.x >= 0 && sector.x < NUM_SECTORS);
		GLASSERT(sector.y >= 0 && sector.y < NUM_SECTORS);
		return coreArr[sector.y*NUM_SECTORS + sector.x]; 
	}

	static CoreScript*  GetCoreFromTeam(int team);
	static CoreScript** GetCoreList(int *n);
	static CoreScript*  CreateCore(const grinliz::Vector2I& sector, int team, const ChitContext* context);

	static void Init();
	static void Free();

private:
	void UpdateAI();
	void UpdateScore(int n);
	bool RecruitNeutral();
	void DoTickNeutral(int delta, int nSpawnTicks);
	void DoTickInUse(int delta, int nSpawnTicks);
	bool SquadAtRest(int squadID);
	void AssignToSquads();
	void DoStrategicTick();
	void NewWaypointChits(int squadID);
	void AssignDefaultSpawns();

	Chit* CitizenFilter(int id);

	WorkQueue*	workQueue;
	grinliz::Vector2I sector;

	CTicker		aiTicker;		// use to update the core info, every couple of seconds.
	CTicker		scoreTicker;
	CTicker		strategicTicker;
	CoreAchievement	achievement;
	
	// serialized
	CTicker		spawnTick;
	double		tech;
	int			summonGreater;
	bool		autoRebuild;
	int			pave;

	grinliz::CDynArray< int > citizens;
	grinliz::CArray<int, SQUAD_SIZE> squads[MAX_SQUADS];
	grinliz::CDynArray< grinliz::Vector2I > tasks;
	
	struct Flag {
		grinliz::Vector2I pos;
		int chitID;

		void Serialize(XStream* xs);
		const bool operator==(const Flag& rhs) { return rhs.pos == pos; }
	};
	grinliz::CDynArray< Flag > flags;

	grinliz::CDynArray<grinliz::Vector2I> waypoints[MAX_SQUADS];

	static CoreScript* coreArr[NUM_SECTORS*NUM_SECTORS];
	static grinliz::HashTable<int, int>* teamToCoreInfo;
	static grinliz::CArray<CoreScript*, NUM_SECTORS*NUM_SECTORS> coreList;
};

#endif // CORESCRIPT_INCLUDED
