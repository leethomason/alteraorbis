#include "sim.h"

#include "../engine/engine.h"
#include "../xegame/xegamelimits.h"
#include "lumosgame.h"
#include "worldmap.h"

Sim::Sim( LumosGame* g )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();
	worldMap = new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine = new Engine( port, database, worldMap );
}


Sim::~Sim()
{
	delete engine;
	delete worldMap;
}
