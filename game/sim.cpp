#include "sim.h"

#include "../engine/engine.h"

#include "../xegame/xegamelimits.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/inventorycomponent.h"

#include "../script/itemscript.h"

#include "pathmovecomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"

#include "lumosgame.h"
#include "worldmap.h"
#include "lumoschitbag.h"

using namespace grinliz;

Sim::Sim( LumosGame* g )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();

	worldMap = new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine = new Engine( port, database, worldMap );

	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	chitBag = new LumosChitBag();
}


Sim::~Sim()
{
	delete chitBag;
	delete engine;
	delete worldMap;
}


void Sim::Load( const char* mapPNG, const char* mapXML )
{
	chitBag->DeleteAll();
	worldMap->Load( mapPNG, mapXML );

	Vector2I v = worldMap->FindEmbark();
	engine->CameraLookAt( (float)v.x, (float)v.y, 20 );
	CreatePlayer( v, "humanFemale" );
}


void Sim::CreatePlayer( const grinliz::Vector2I& pos, const char* assetName )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get( assetName, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = chitBag->NewChit();
	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, assetName ));
	chit->Add( new PathMoveComponent( worldMap ));
	chit->Add( new DebugStateComponent( worldMap ));

	GameItem item( *(itemDefArr[0]));
	item.primaryTeam = 1;
	item.stats.SetExpFromLevel( 4 );
	item.InitState();
	chit->Add( new ItemComponent( item ));

	chit->Add( new HealthComponent());
	chit->Add( new InventoryComponent());

	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );
}


Texture* Sim::GetMiniMapTexture()
{
	return engine->GetMiniMapTexture();
}


void Sim::DoTick( U32 delta )
{
	chitBag->DoTick( delta, engine );
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}



