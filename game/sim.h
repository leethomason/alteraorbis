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

#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcontainer.h"

#include "gamelimits.h"
#include "visitor.h"
#include "visitorweb.h"
#include "team.h"

#include "../xegame/cticker.h"
#include "../xegame/chit.h"
#include "../xegame/stackedsingleton.h"
#include "../xegame/chitbag.h"
#include "../xegame/chitcontext.h"

#include "../engine/engine.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class Texture;
class Chit;
class ChitMsg;
class Weather;
class ReserveBank;
class NewsHistory;
class ItemDB;
class CoreScript;
class PlantScript;
class CircuitSim;
class Team;

class Sim : public IChitListener, public IUITracker
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime, bool useAreaOfInterest=true );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapDAT, const char* gameDAT );
	void Save( const char* mapDAT, const char* gameDAT );

	Texture*		GetMiniMapTexture();

	// use with caution: not a clear separation between sim and game
	const ChitContext* Context()	{ return &context; }
	Engine*			GetEngine()		{ return context.engine; }
	LumosChitBag*	GetChitBag()	{ return context.chitBag; }
	WorldMap*		GetWorldMap()	{ return context.worldMap; }

	int    AgeI() const;
	double AgeD() const;
	float  AgeF() const;

	enum {
		SPAWN_DENIZENS = 0x01,
		SPAWN_LESSER = 0x02,
		SPAWN_GREATER = 0x04,
	};
	int SpawnEnabled() const			{ return spawnEnabled; }
	void EnableSpawn(int enable)		{ spawnEnabled = enable;  }

	// Set all rock to the nominal values
	void SetAllRock();
	void CreateVolcano( int x, int y );
	// type=-1 will scan for natural plant choice
	bool CreatePlant( int x, int y, int type, int stage=0 );
	void SeedPlants();
	
	void CreateAvatar( const grinliz::Vector2I& pos );
	void UseBuilding();	// the player wants to use a building

	// IChitListener.
	// Listens for cores to be destroyed and re-creates.
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);

	// IUITracker
	void UpdateUIElements(const Model* model[], int nModels);

	const Web& GetCachedWeb();
	const Web& CalcWeb();

	void CalcStrategicRelationships(const grinliz::Vector2I& sector, int radius, ERelate relate, grinliz::CArray<CoreScript*, 32> *stateArr);
	void DeclareWar(CoreScript* target, CoreScript* src)	{}	// does nothing, yet

private:
	void CreateCores();
	void CreateRockInOutland();
	void DoWeatherEffects( U32 delta );
	void DumpModel();

	void SpawnGreater();
	void SpawnDenizens();
	void CreateTruulgaCore();
	
	ChitContext		context;
	Weather*		weather;
	ReserveBank*	reserveBank;
	Team*			teamInfo;
	Visitors*		visitors;
	ItemDB*			itemDB;
	PlantScript*	plantScript;

	grinliz::Random	random;
	int avatarTimer;
	CTicker minuteClock, 
			secondClock, 
			volcTimer,
			spawnClock,
			visitorClock;
	int currentVisitor;
	int spawnEnabled;

	int cachedWebAge;
	Web web;

	grinliz::CDynArray< Chit* >	queryArr;					// local; cached at object.
	grinliz::CDynArray< grinliz::Vector2I > coreCreateList;	// list of cores that were deleted, need to be re-created after DoTick
	grinliz::CDynArray< int > uiChits;						// chits that have displayed UI elements
};


#endif // LUMOS_SIM_INCLUDED
