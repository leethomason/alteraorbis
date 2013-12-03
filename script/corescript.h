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

class WorldMap;
class WorkQueue;
class LumosChitBag;

class CoreScript : public IScript
{
public:
	CoreScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine );
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
	int  NumCitizens();

	bool InUse() const;
	int  PrimaryTeam() const;

	WorkQueue* GetWorkQueue()		{ return workQueue; }
	void SetDefaultSpawn( grinliz::IString s ) { defaultSpawn = s; }

	int GetTechLevel() const		{ return (int)GetTech(); }
	int AchievedTechLevel() const	{ return achievedTechLevel; }

	float GetTech() const			{ return float(tech); }
	int   MaxTech() const;
	void  AddTech();

private:
	WorldMap*	worldMap;
	WorkQueue*	workQueue;
	int			team;			// cache so we can update if it changes
	
	// serialized
	CTicker		spawnTick;
	double		tech;
	int			achievedTechLevel;
	grinliz::IString defaultSpawn;
	grinliz::CDynArray< int > citizens;
	mutable grinliz::CDynArray< Chit* > chitArr;	// used as a temporary, memory cached here.
};

#endif // CORESCRIPT_INCLUDED
