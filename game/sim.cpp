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
#include "../script/scriptcomponent.h"
#include "../script/volcanoscript.h"

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
	minuteClock = 0;
	timeInMinutes = 0;
	volcTimer = 0;

	random.SetSeedFromTime();
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
			Archive( 0, root );
			engine->camera.Load( root );
			chitBag->Load( &factory, root );
		}
	}
}


void Sim::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( playerID );
	XE_ARCHIVE( minuteClock );
	XE_ARCHIVE( timeInMinutes );
}


void Sim::Save( const char* mapDAT, const char* mapXML, const char* gameXML )
{
	worldMap->Save( mapDAT, mapXML );

	FILE* fp = fopen( gameXML, "w" );
	GLASSERT( fp );
	if ( fp ) {
		XMLPrinter printer( fp );
		printer.OpenElement( "Sim" );
		Archive( &printer, 0 );
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
	worldMap->DoTick( delta, chitBag );
	chitBag->DoTick( delta, engine );

	bool minuteTick = false;

	minuteClock -= (int)delta;
	if ( minuteClock <= 0 ) {
		minuteClock += MINUTE;
		timeInMinutes++;
		minuteTick = true;
	}

	// Logic that will probably need to be broken out.
	// What happens in a given age?
	int age = minuteClock / AGE;
	volcTimer += delta;

	// Age of Fire
	static const int VOLC_RAD = 9;
	static const int VOLC_DIAM = VOLC_RAD*2+1;
	static const int NUM_VOLC = MAX_MAP_SIZE*MAX_MAP_SIZE / (VOLC_DIAM*VOLC_DIAM);
	static const int MSEC_TO_VOLC = AGE / NUM_VOLC;

	switch( age ) {
	case 0:
		// The Age of Fire
		// Essentially want to get the world covered.
		while( volcTimer >= MSEC_TO_VOLC ) {
			volcTimer -= MSEC_TO_VOLC;

			for( int i=0; i<5; ++i ) {
				int x = random.Rand(worldMap->Width());
				int y = random.Rand(worldMap->Height());
				if ( worldMap->IsLand( x, y ) ) {
					CreateVolcano( x, y, VOLC_RAD );
					break;
				}
			}
		}
		break;

	default:
		if ( minuteTick && random.Rand(3)==0 ) {
			CreateVolcano( random.Rand(worldMap->Width()), random.Rand(worldMap->Height()), VOLC_RAD );
		}
		break;
	}
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}


void Sim::SetAllRock()
{
	for( int j=0; j<worldMap->Height(); ++j ) {
		for( int i=0; i<worldMap->Width(); ++i ) {
			worldMap->SetRock( i, j, -1, 0, false );
		}
	}
}


void Sim::CreateVolcano( int x, int y, int size )
{
	Chit* chit = chitBag->NewChit();
	chit->Add( new SpatialComponent() );
	chit->Add( new ScriptComponent( new VolcanoScript( worldMap, size )));

	chit->GetSpatialComponent()->SetPosition( (float)x+0.5f, 0.0f, (float)y+0.5f );
}

