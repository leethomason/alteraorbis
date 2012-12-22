#include "sim.h"

#include "../engine/engine.h"
#include "../xegame/xegamelimits.h"
#include "lumosgame.h"
#include "worldmap.h"

using namespace grinliz;

Sim::Sim( LumosGame* g )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();
	worldMap = new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine = new Engine( port, database, worldMap );
	engine->SetGlow( true );
}


Sim::~Sim()
{
	delete engine;
	delete worldMap;
}


void Sim::Load( const char* mapPNG, const char* mapXML )
{
	worldMap->Load( mapPNG, mapXML );
	Vector2I v = worldMap->FindEmbark();
	engine->CameraLookAt( (float)v.x, (float)v.y, 30 );
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}



