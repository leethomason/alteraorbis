#include "sim.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/xegamelimits.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/componentfactory.h"
#include "../xegame/cameracomponent.h"

#include "../script/itemscript.h"
#include "../script/scriptcomponent.h"
#include "../script/volcanoscript.h"
#include "../script/plantscript.h"
#include "../script/corescript.h"
#include "../script/worldgen.h"

#include "pathmovecomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"
#include "lumosgame.h"
#include "worldmap.h"
#include "lumoschitbag.h"
#include "weather.h"
#include "mapspatialcomponent.h"
#include "aicomponent.h"
#include "reservebank.h"
#include "team.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

Sim::Sim( LumosGame* g )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();

	worldMap	= new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine		= new Engine( port, database, worldMap );
	weather		= new Weather( MAX_MAP_SIZE, MAX_MAP_SIZE );
	reserveBank = new ReserveBank();
	visitors	= new Visitors();

	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	chitBag = new LumosChitBag();
	chitBag->SetContext( engine, worldMap );
	worldMap->AttachEngine( engine, chitBag );
	playerID = 0;
	minuteClock = 0;
	timeInMinutes = 0;
	volcTimer = 0;
	secondClock = 1000;
	currentVisitor = 0;

	random.SetSeedFromTime();
}


Sim::~Sim()
{
	delete visitors;
	delete weather;
	delete reserveBank;
	worldMap->AttachEngine( 0, 0 );
	delete chitBag;
	delete engine;
	delete worldMap;
}


void Sim::Load( const char* mapDAT, const char* gameDAT )
{
	chitBag->DeleteAll();
	worldMap->Load( mapDAT );

	if ( !gameDAT ) {
		// Fresh start
		CreateRockInOutland();
		CreateCores();
		CreatePlayer();
	}
	else {
		QuickProfile qp( "Sim::Load" );
		ComponentFactory factory( this, &chitBag->census, engine, worldMap, chitBag, weather, lumosGame );

		FILE* fp = fopen( gameDAT, "rb" );
		GLASSERT( fp );
		if ( fp ) {
			StreamReader reader( fp );
			XarcOpen( &reader, "Sim" );
			XARC_SER( &reader, playerID );
			XARC_SER( &reader, minuteClock );
			XARC_SER( &reader, timeInMinutes );

			reserveBank->Serialize( &reader );
			visitors->Serialize( &reader );
			engine->camera.Serialize( &reader );
			chitBag->Serialize( &factory, &reader );

			XarcClose( &reader );

			fclose( fp );
		}
	}
	if ( chitBag->GetChit( playerID )) {
		// mark as player controlled so it reacts as expected to player input.
		chitBag->GetChit( playerID )->GetAIComponent()->playerControlled = true;
	}
}


void Sim::Save( const char* mapDAT, const char* gameDAT )
{
	worldMap->Save( mapDAT );

	{
		QuickProfile qp( "Sim::SaveXarc" );

		ComponentFactory factory( this, &chitBag->census, engine, worldMap, chitBag, weather, lumosGame );

		FILE* fp = fopen( gameDAT, "wb" );
		if ( fp ) {
			StreamWriter writer( fp );
			XarcOpen( &writer, "Sim" );
			XARC_SER( &writer, playerID );
			XARC_SER( &writer, minuteClock );
			XARC_SER( &writer, timeInMinutes );

			reserveBank->Serialize( &writer );
			visitors->Serialize( &writer );
			engine->camera.Serialize( &writer );
			chitBag->Serialize( &factory, &writer );

			XarcClose( &writer );
			fclose( fp );
		}
	}
}


void Sim::CreateCores()
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem& coreItem = itemDefDB->Get( "core" );

	const SectorData* sectorDataArr = worldMap->GetSectorData();

	int ncores = 0;

	for( int i=0; i<NUM_SECTORS*NUM_SECTORS; ++i ) {
		const SectorData& sd = sectorDataArr[i];
		if ( sd.HasCore() ) {
			Chit* chit = chitBag->NewBuilding( sd.core, "core", 0 );

			// 'in use' instead of blocking.
			MapSpatialComponent* ms = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
			GLASSERT( ms );
			ms->SetMode( GRID_IN_USE ); 
			chit->Add( new ScriptComponent( new CoreScript( worldMap, chitBag, engine ), engine, &chitBag->census ));
			++ncores;
		}
	}
	GLOUTPUT(( "nCores=%d\n", ncores ));
}


void Sim::CreatePlayer()
{
	Vector2I v = worldMap->FindEmbark();
	CreatePlayer( v, "humanFemale" );
}


void Sim::CreatePlayer( const grinliz::Vector2I& pos, const char* assetName )
{
	if ( !assetName ) {
		assetName = "humanFemale";
	}

	Chit* chit = chitBag->NewChit();
	playerID = chit->ID();
	chitBag->GetCamera( engine )->SetTrack( playerID );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, assetName ));
	chit->Add( new PathMoveComponent( worldMap ));

	chitBag->AddItem( assetName, chit, engine, TEAM_HOUSE0, 0 );
	chitBag->AddItem( "shield", chit, engine, 0, 2 );
	chitBag->AddItem( "blaster", chit, engine, 0, 2 );
	chit->GetItemComponent()->AddGold( ReserveBank::Instance()->WithdrawDenizen() );
	chit->GetItem()->flags |= GameItem::AI_BINDS_TO_CORE;

	AIComponent* ai = new AIComponent( engine, worldMap );
	ai->EnableDebug( true );
	ai->playerControlled = true;
	chit->Add( ai );

	chit->Add( new HealthComponent( engine ));
	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );
	chit->GetItemComponent()->SetProceduralHardpoints();
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
	bool secondTick = false;

	minuteClock -= (int)delta;
	if ( minuteClock <= 0 ) {
		minuteClock += MINUTE;
		timeInMinutes++;
		minuteTick = true;
	}

	secondClock -= (int)delta;
	if ( secondClock <= 0 ) {
		secondClock += 1000;
		secondTick = true;
	}

	// Logic that will probably need to be broken out.
	// What happens in a given age?
	int age = timeInMinutes / MINUTES_IN_AGE;
	volcTimer += delta;

	// Age of Fire. Needs lots of volcanoes to seed the world.
	static const int VOLC_RAD = 9;
	static const int VOLC_DIAM = VOLC_RAD*2+1;
	static const int NUM_VOLC = MAX_MAP_SIZE*MAX_MAP_SIZE / (VOLC_DIAM*VOLC_DIAM);
	int MSEC_TO_VOLC = AGE / NUM_VOLC;

	// NOT Age of Fire:
	if ( age > 1 ) {
		MSEC_TO_VOLC *= 20;
	}
	else if ( age > 0 ) {
		MSEC_TO_VOLC *= 10;
	}

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

	if ( secondTick ) {
		VisitorData* visitorData = Visitors::Instance()->visitorData;

		// Check if the visitor is still in the world.
		if ( visitorData[currentVisitor].id ) {
			if ( !chitBag->GetChit( visitorData[currentVisitor].id )) {
				visitorData[currentVisitor].id = 0;
			}
		}

		if ( visitorData[currentVisitor].id == 0 ) {
			Chit* chit = chitBag->NewVisitor( currentVisitor );
			visitorData[currentVisitor].id = chit->ID();

#if 0
			NewsEvent news( NewsEvent::PONY, chit->GetSpatialComponent()->GetPosition2D(), 
							StringPool::Intern( "Visitor" ), chit->ID() );
			chitBag->AddNews( news );
			chit->GetAIComponent()->EnableDebug( true );
#endif
		}

		currentVisitor++;
		if ( currentVisitor == Visitors::NUM_VISITORS ) {
			currentVisitor = 0;
		}
	}

	CreatePlant( random.Rand(worldMap->Width()), random.Rand(worldMap->Height()), -1 );

	DoWeatherEffects( delta );
}


void Sim::DoWeatherEffects( U32 delta )
{
	Vector3F at;
	engine->CameraLookingAt( &at );

	float rain = weather->RainFraction( at.x, at.z );

	if ( rain > 0.5f ) {
		float rainEffect = (rain-0.5f)*2.0f;	// 0-1

		ParticleDef pd = engine->particleSystem->GetPD( "splash" );
		const float rad = (at - engine->camera.PosWC()).Length() * sinf( ToRadian( EL_FOV ));
		const float RAIN_AREA = rad * rad;
		pd.count = (int)(RAIN_AREA * rainEffect * 0.05f );

		/*
			x rain splash	
			x desaturation
			- fog
		*/

		Rectangle3F rainBounds;
		rainBounds.Set( at.x-rad, 0.02f, at.z-rad, at.x+rad, 0.02f, at.z+rad );
		worldMap->SetSaturation( 1.0f - 0.50f*rainEffect );

		engine->particleSystem->EmitPD( pd, rainBounds, V3F_UP, delta );
	}
	else {
		worldMap->SetSaturation( 1 );
	}
	float normalTemp = weather->Temperature( at.x, at.z ) * 2.0f - 1.0f;
	engine->SetTemperature( normalTemp );
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime, chitBag->BoltMem(), chitBag->NumBolts() );
}


void Sim::CreateRockInOutland()
{
	const SectorData* sectorDataArr = worldMap->GetSectorData();
	for( int i=0; i<NUM_SECTORS*NUM_SECTORS; ++i ) {
		const SectorData& sd = sectorDataArr[i];
		if ( !sd.HasCore() ) {
			for( int j=sd.y; j<sd.y+SECTOR_SIZE; ++j ) {
				for( int i=sd.x; i<sd.x+SECTOR_SIZE; ++i ) {
					worldMap->SetRock( i, j, -1, false, 0 );
				}
			}
		}
	}
}


void Sim::SetAllRock()
{
	for( int j=0; j<worldMap->Height(); ++j ) {
		for( int i=0; i<worldMap->Width(); ++i ) {
			worldMap->SetRock( i, j, -1, false, 0 );
		}
	}
}


void Sim::CreateVolcano( int x, int y, int size )
{
	const SectorData& sd = worldMap->GetSector( x, y );
	if ( sd.ports == 0 ) {
		// no point to volcanoes in the outland
		return;
	}

	Chit* chit = chitBag->NewChit();
	chit->Add( new SpatialComponent() );
	chit->Add( new ScriptComponent( new VolcanoScript( worldMap, size ), engine, &chitBag->census ));

	chit->GetSpatialComponent()->SetPosition( (float)x+0.5f, 0.0f, (float)y+0.5f );
}


void Sim::CreatePlant( int x, int y, int type )
{
	if ( !worldMap->Bounds().Contains( x, y ) ) {
		return;
	}
	const SectorData& sd = worldMap->GetSector( x, y );
	if ( sd.ports == 0 ) {
		// no point to plants in the outland
		return;
	}

	// About 50,000 plants seems about right.
	int count = chitBag->census.CountPlants();
	if ( count > worldMap->Bounds().Area() / 20 ) {
		return;
	}

	// And space them out a little.
	Random r( x+y*1024 );
	if ( r.Rand(4) == 0 )
		return;

	const WorldGrid& wg = worldMap->GetWorldGrid( x, y );

	// check for a plant already there.
	if (    wg.IsPassable() 
		 && wg.Layer() == WorldGrid::LAND 
		 && !chitBag->MapGridUse(x,y) ) 
	{
		if ( type < 0 ) {
			// Scan for a good type!
			Rectangle2F r;
			r.min.Set( (float)x+0.5f, (float)y+0.5f );
			r.max = r.min;
			r.Outset( 3.0f );

			PlantFilter plantFilter;
			chitBag->QuerySpatialHash( &queryArr, r, 0, &plantFilter );

			float chance[NUM_PLANT_TYPES];
			for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
				chance[i] = 1.0f;
			}

			for( int i=0; i<queryArr.Size(); ++i ) {
				Vector3F pos = { (float)x+0.5f, 0, (float)y+0.5f };
				Chit* c = queryArr[i];

				int stage, type;
				GameItem* item = PlantScript::IsPlant( c, &type, &stage );
				if ( item ) {
					float weight = (float)((stage+1)*(stage+1));
					chance[type] += weight;
				}
			}
			type = random.Select( chance, NUM_PLANT_TYPES );
		}

		Chit* chit = chitBag->NewChit();
		MapSpatialComponent* ms = new MapSpatialComponent( worldMap );
		ms->SetMapPosition( x, y, 1, 1 );
		chit->Add( ms );

		chit->Add( new HealthComponent( engine ) );
		chit->Add( new ScriptComponent( new PlantScript( this, engine, worldMap, weather, type ), engine, &chitBag->census ));
	}
}

