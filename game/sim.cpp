#include "sim.h"

#include "../engine/engine.h"

#include "../xegame/xegamelimits.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/inventorycomponent.h"
#include "../xegame/componentfactory.h"

#include "../script/itemscript.h"

#include "pathmovecomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"

#include "lumosgame.h"
#include "worldmap.h"
#include "lumoschitbag.h"

using namespace grinliz;
using namespace tinyxml2;

Sim::Sim( LumosGame* g )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();

	worldMap = new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine = new Engine( port, database, worldMap );
	worldMap->AttachEngine( engine );

	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	chitBag = new LumosChitBag();
	playerID = 0;
}


Sim::~Sim()
{
	worldMap->AttachEngine( 0 );
	delete chitBag;
	delete engine;
	delete worldMap;
}


void Sim::Load( const char* mapDAT, const char* mapXML, const char* gameXML )
{
	chitBag->DeleteAll();
	worldMap->Load( mapDAT, mapXML );

	if ( !gameXML ) {
		Vector2I v = worldMap->FindEmbark();
		CreatePlayer( v, "humanFemale" );
	}
	else {
		ComponentFactory factory( engine, worldMap, lumosGame );
		XMLDocument doc;
		doc.LoadFile( gameXML );
		GLASSERT( !doc.Error() );
		if ( !doc.Error() ) {
			const XMLElement* root = doc.FirstChildElement( "Sim" );
			playerID = 0;
			root->QueryAttribute( "playerID", &playerID );
			engine->camera.Load( root );
			chitBag->Load( &factory, root );
		}
	}
}


void Sim::Save( const char* mapDAT, const char* mapXML, const char* gameXML )
{
	worldMap->Save( mapDAT, mapXML );

	FILE* fp = fopen( gameXML, "w" );
	GLASSERT( fp );
	if ( fp ) {
		XMLPrinter printer( fp );
		printer.OpenElement( "Sim" );
		printer.PushAttribute( "playerID", playerID );
		engine->camera.Save( &printer );
		chitBag->Save( &printer );
		printer.CloseElement();
		fclose( fp );
	}
}


void Sim::CreatePlayer( const grinliz::Vector2I& pos, const char* assetName )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get( assetName, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = chitBag->NewChit();
	playerID = chit->ID();

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


Chit* Sim::GetPlayerChit()
{
	return chitBag->GetChit( playerID );
}


Texture* Sim::GetMiniMapTexture()
{
	return engine->GetMiniMapTexture();
}


void Sim::DoTick( U32 delta )
{
	chitBag->DoTick( delta, engine );
	worldMap->DoTick();
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}


void Sim::SetAllRock()
{
	for( int j=0; j<worldMap->Height(); ++j ) {
		for( int i=0; i<worldMap->Width(); ++i ) {
			worldMap->SetRock( i, j, -1, 0 );
		}
	}
}


