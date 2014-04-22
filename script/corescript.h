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

#include "scriptcomponent.h"
#include "../xegame/cticker.h"
#include "../game/gamelimits.h"

class WorldMap;
class WorkQueue;
class LumosChitBag;

struct CoreInfo
{
	CoreScript* coreScript;	

	// these are value for ai - so they are only updated every couple of seconds.
	//int approxTeam;
	//int approxMaxTech;	// 1-4
};


struct CoreAchievement
{
	void Clear() { techLevel = 0; gold = 0; population = 0; civTechScore = 0; }
	void Serialize(XStream* xs);

	int			techLevel;
	int			gold;
	int			population;
	int			civTechScore;
};


class CoreScript : public IScript
{
public:
	CoreScript();
	virtual ~CoreScript();

	virtual void Init();
	virtual void Serialize( XStream* xs );
	virtual void OnAdd();
	virtual void OnRemove();

	virtual int DoTick( U32 delta );
	virtual const char* ScriptName()	{ return "CoreScript"; }
	virtual CoreScript* ToCoreScript()	{ return this; }

	void AddCitizen( Chit* chit );
	bool IsCitizen( Chit* chit ); 
	bool IsCitizen( int id );
	Chit*  CitizenAtIndex( int id );
	int    FindCitizenIndex( Chit* chit ); 
	int  NumCitizens();

	bool InUse() const;
	int  PrimaryTeam() const;

	WorkQueue* GetWorkQueue()		{ return workQueue; }
	void SetDefaultSpawn( grinliz::IString s ) { defaultSpawn = s; }

	int GetTechLevel() const		{ return (int)GetTech(); }
	int AchievedTechLevel() const	{ return achievement.techLevel; }
	const CoreAchievement& GetAchievement() const { return achievement; }

	float GetTech() const			{ return float(tech); }
	int   MaxTech() const;
	void  AddTech();

	int nElixir;

	// Testing: each task pushes a position for that task,
	// and removes it when done/cancelled. (Some careful
	// code there.) May be a simpler way to see if
	// someone is assigned to a task at location. However,
	// approximate, since nothing but the location is tracked.
	void AddTask(const grinliz::Vector2I& pos2i);
	void RemoveTask(const grinliz::Vector2I& pos2i);
	bool HasTask(const grinliz::Vector2I& pos2i);

	static const CoreInfo& GetCoreInfo(const grinliz::Vector2I& sector) { 
		GLASSERT(sector.x >= 0 && sector.x < NUM_SECTORS);
		GLASSERT(sector.y >= 0 && sector.y < NUM_SECTORS);
		return coreInfoArr[sector.y*NUM_SECTORS + sector.x]; 
	}

	static CoreScript* GetCore(const grinliz::Vector2I& sector) {
		return GetCoreInfo(sector).coreScript;
	}
	static const CoreInfo* GetCoreInfoArr() { return coreInfoArr; }

private:
	void UpdateAI();
	void UpdateScore(int n);

	WorkQueue*	workQueue;
	int			team;			// cache so we can update if it changes
	grinliz::Vector2I sector;

	CTicker		aiTicker;		// use to update the core info, every couple of seconds.
	CTicker		scoreTicker;
	CoreAchievement	achievement;
	
	// serialized
	CTicker		spawnTick;
	double		tech;
	int			summonGreater;

	grinliz::IString defaultSpawn;
	grinliz::CDynArray< int > citizens;
	grinliz::CDynArray< grinliz::Vector2I > tasks;

	static CoreInfo coreInfoArr[NUM_SECTORS*NUM_SECTORS];
};

#endif // CORESCRIPT_INCLUDED
