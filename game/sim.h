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
#include "../xegame/cticker.h"
#include "../xegame/stackedsingleton.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class Texture;
class Chit;
class Weather;
class ReserveBank;
class NewsHistory;
class ItemDB;

class Sim : public StackedSingleton<Sim>
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapDAT, const char* gameDAT );
	void Save( const char* mapDAT, const char* gameDAT );

	void Draw( U32 delta );

	Texture*		GetMiniMapTexture();
	Chit*			GetPlayerChit();

	// use with caution: not a clear separation between sim and game
	Engine*			GetEngine()		{ return engine; }
	LumosChitBag*	GetChitBag()	{ return chitBag; }
	WorldMap*		GetWorldMap()	{ return worldMap; }

	// Set all rock to the nominal values
	void SetAllRock();
	void CreateVolcano( int x, int y, int size );
	// type=-1 will scan for natural plant choice
	Chit* CreatePlant( int x, int y, int type );
	
	void CreatePlayer();
	void CreatePlayer( const grinliz::Vector2I& pos );
	void UseBuilding();	// the player wants to use a building

private:
	void CreateCores();
	void CreateRockInOutland();
	void DoWeatherEffects( U32 delta );
	
	Engine*			engine;
	LumosGame*		lumosGame;
	WorldMap*		worldMap;
	Weather*		weather;
	ReserveBank*	reserveBank;
	LumosChitBag*	chitBag;
	Visitors*		visitors;
	NewsHistory*	newsHistory;
	ItemDB*			itemDB;

	grinliz::Random	random;
	int playerID;
	CTicker minuteClock, 
			secondClock, 
			volcTimer;
	int currentVisitor;

	grinliz::CDynArray< Chit* >	queryArr;	// local; cached at object.
};


#endif // LUMOS_SIM_INCLUDED
