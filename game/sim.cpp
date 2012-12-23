#include "sim.h"

#include "../engine/engine.h"
#include "../xegame/xegamelimits.h"
#include "../xegame/chitbag.h"
#include "../script/itemscript.h"
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

	chitBag = new ChitBag();
	itemStorage = 0;
}


Sim::~Sim()
{
	delete itemStorage;
	delete chitBag;
	delete engine;
	delete worldMap;
}


void Sim::Load( const char* mapPNG, const char* mapXML, const char* itemDefPath )
{
	worldMap->Load( mapPNG, mapXML );
	Vector2I v = worldMap->FindEmbark();
	engine->CameraLookAt( (float)v.x, (float)v.y, 50 );

	chitBag->DeleteAll();

	delete itemStorage;
	itemStorage = new ItemStorage();
	itemStorage->Load( itemDefPath );
}


void Sim::CreatePlayer( const grinliz::Vector2I& pos, const char* assetName )
{
	ItemStorage::GameItemArr itemDefArr;
	itemStorage.Get( assetName, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, asset ));
	chit->Add( new PathMoveComponent( map ));
	chit->Add( new DebugStateComponent( map ));

	GameItem item( *(itemDefArr[0]));
	item.primaryTeam = 1;
	item.stats.SetExpFromLevel( 4 );
	item.InitState();
	chit->Add( new ItemComponent( item ));

	chit->Add( new HealthComponent());
	chit->Add( new InventoryComponent());

	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}



