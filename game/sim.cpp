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
#include "../xegame/istringconst.h"

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
#include "lumosmath.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

Sim::Sim( LumosGame* g ) : minuteClock( 60*1000 ), secondClock( 1000 ), volcTimer( 10*1000 )
{
	lumosGame = g;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();

	itemDB		= new ItemDB();
	worldMap	= new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine		= new Engine( port, database, worldMap );
	weather		= new Weather( MAX_MAP_SIZE, MAX_MAP_SIZE );
	reserveBank = new ReserveBank();
	visitors	= new Visitors();

	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	chitBag = new LumosChitBag();
	chitBag->SetContext( engine, worldMap, lumosGame );
	newsHistory = new NewsHistory( chitBag );
	worldMap->AttachEngine( engine, chitBag );
	playerID = 0;
	currentVisitor = 0;

	random.SetSeedFromTime();
}


Sim::~Sim()
{
	delete visitors;
	delete weather;
	delete reserveBank;
	worldMap->AttachEngine( 0, 0 );
	delete newsHistory;
	delete chitBag;
	delete engine;
	delete worldMap;
	delete itemDB;
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
		//QuickProfile qp( "Sim::Load" );
		ComponentFactory factory( this, &chitBag->census, engine, worldMap, chitBag, weather, lumosGame );

		FILE* fp = fopen( gameDAT, "rb" );
		GLASSERT( fp );
		if ( fp ) {
			StreamReader reader( fp );
			XarcOpen( &reader, "Sim" );

			XARC_SER( &reader, playerID );
			XARC_SER( &reader, GameItem::idPool );

			minuteClock.Serialize( &reader, "minuteClock" );
			secondClock.Serialize( &reader, "secondClock" );
			volcTimer.Serialize( &reader, "volcTimer" );
			itemDB->Serialize( &reader );
			newsHistory->Serialize( &reader );
			reserveBank->Serialize( &reader );
			visitors->Serialize( &reader );
			engine->camera.Serialize( &reader );
			chitBag->Serialize( &factory, &reader );

			XarcClose( &reader );

			fclose( fp );
		}
	}
	if ( chitBag->GetChit( playerID )) {
		// Mark as player controlled so it reacts as expected to player input.
		// This is the primary avatar, and has some special rules.
		chitBag->GetChit( playerID )->SetPlayerControlled( true );
	}
}


void Sim::Save( const char* mapDAT, const char* gameDAT )
{
	worldMap->Save( mapDAT );

	{
		//QuickProfile qp( "Sim::SaveXarc" );

		ComponentFactory factory( this, &chitBag->census, engine, worldMap, chitBag, weather, lumosGame );

		FILE* fp = fopen( gameDAT, "wb" );
		if ( fp ) {
			StreamWriter writer( fp );
			XarcOpen( &writer, "Sim" );
			XARC_SER( &writer, playerID );
			XARC_SER( &writer, GameItem::idPool );

			minuteClock.Serialize( &writer, "minuteClock" );
			secondClock.Serialize( &writer, "secondClock" );
			volcTimer.Serialize( &writer, "volcTimer" );
			itemDB->Serialize( &writer );
			newsHistory->Serialize( &writer );
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
	CoreScript* cold = 0;
	CoreScript* hot  = 0;

	for( int i=0; i<NUM_SECTORS*NUM_SECTORS; ++i ) {
		const SectorData& sd = sectorDataArr[i];
		if ( sd.HasCore() ) {
			Chit* chit = chitBag->NewBuilding( sd.core, "core", 0 );

			// 'in use' instead of blocking.
			MapSpatialComponent* ms = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
			GLASSERT( ms );
			ms->SetMode( GRID_IN_USE ); 
			CoreScript* cs = new CoreScript( worldMap, chitBag, engine );
			chit->Add( new ScriptComponent( cs, engine, &chitBag->census ));

			if ( !cold ) cold = cs;	// first
			hot = cs;	// last
			++ncores;
		}
	}
	cold->SetDefaultSpawn( StringPool::Intern( "arachnoid" ));	// easy starting point for greater spawns
	hot->SetDefaultSpawn( StringPool::Intern( "arachnoid" ));	// easy starting point for greater spawns
	GLOUTPUT(( "nCores=%d\n", ncores ));

	Vector2I homeSector = chitBag->GetHomeSector();
	CoreScript* homeCS = chitBag->GetCore( homeSector );
	Chit* homeChit = homeCS->ParentChit();
	homeChit->GetItem()->primaryTeam = TEAM_HOUSE0;
}


void Sim::CreatePlayer()
{
	//Vector2I v = worldMap->FindEmbark();

	Vector2I sector = chitBag->GetHomeSector();
	Vector2I v = {	sector.x*SECTOR_SIZE + SECTOR_SIZE/2 + 4,
					sector.y*SECTOR_SIZE + SECTOR_SIZE/2 + 4 };
	CreatePlayer( v );
}


void Sim::CreatePlayer( const grinliz::Vector2I& pos )
{
	bool female = true;
	const char* assetName = "humanFemale";
	if ( random.Bit() ) {
		female = false;
		assetName = "humanMale";
	}

	Chit* chit = chitBag->NewChit();
	chit->SetPlayerControlled( true );
	playerID = chit->ID();
	chitBag->GetCamera( engine )->SetTrack( playerID );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, assetName ));
	chit->Add( new PathMoveComponent( worldMap ));

	chitBag->AddItem( assetName, chit, engine, TEAM_HOUSE0, 0 );
	chitBag->AddItem( "shield", chit, engine, 0, 0 );
	chitBag->AddItem( "blaster", chit, engine, 0, 0 );
	chitBag->AddItem( "ring", chit, engine, 0, 0 );
	chit->GetItem()->wallet.AddGold( ReserveBank::Instance()->WithdrawDenizen() );
	chit->GetItem()->traits.Roll( playerID );

	IString nameGen = chit->GetItem()->keyValues.GetIString( "nameGen" );
	if ( !nameGen.empty() ) {
		chit->GetItem()->SetProperName( StringPool::Intern( 
			lumosGame->GenName( nameGen.c_str(), 
								chit->ID(),
								4, 8 )));
	}
	AIComponent* ai = new AIComponent( engine, worldMap );
	ai->EnableDebug( true );
	chit->Add( ai );

	chit->Add( new HealthComponent( engine ));
	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );
//	chit->GetItemComponent()->SetHardpoints(); automatic now

	// Player speed boost
	chit->GetItem()->keyValues.Set( "speed", "f", DEFAULT_MOVE_SPEED*1.5f/1.2f );
	chit->GetItem()->hpRegen = 1.0f;

	Vector2I sector = ToSector( pos );
	chitBag->GetCore( sector )->AddCitizen( chit );

	NewsHistory* history = NewsHistory::Instance();
	if ( history ) {
		history->Add( NewsEvent( NewsEvent::DENIZEN_CREATED, ToWorld2F(pos), chit, 0 ));
		chit->GetItem()->keyValues.Set( "destroyMsg", "d", NewsEvent::DENIZEN_KILLED );
	}
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
	NewsHistory::Instance()->DoTick( delta );
	worldMap->DoTick( delta, chitBag );
	chitBag->DoTick( delta, engine );

	int minuteTick = minuteClock.Delta( delta );
	int secondTick = secondClock.Delta( delta );
	int volcano    = volcTimer.Delta( delta );

	// Age of Fire. Needs lots of volcanoes to seed the world.
	static const int VOLC_RAD = 9;
	static const int VOLC_DIAM = VOLC_RAD*2+1;
	static const int NUM_VOLC = MAX_MAP_SIZE*MAX_MAP_SIZE / (VOLC_DIAM*VOLC_DIAM);
	int MSEC_TO_VOLC = AGE_IN_MSEC / NUM_VOLC;

	int age = NewsHistory::Instance()->AgeI();

	// NOT Age of Fire:
	volcTimer.SetPeriod( (age > 1 ? MSEC_TO_VOLC*4 : MSEC_TO_VOLC) + random.Rand(1000) );

	while( volcano-- ) {
		for( int i=0; i<5; ++i ) {
			int x = random.Rand(worldMap->Width());
			int y = random.Rand(worldMap->Height());
			if ( worldMap->IsLand( x, y ) ) {
				// Don't destroy opporating domains. Crazy annoying.
				Vector2I pos = { x, y };
				Vector2I sector = ToSector( pos );
				CoreScript* cs = chitBag->GetCore( sector );
				if ( cs && !cs->InUse() ) {
					CreateVolcano( x, y, VOLC_RAD );
				}
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

	// Special rule for player controlled chit: give money to the core.
	CoreScript* cs = chitBag->GetHomeCore();
	Chit* player   = this->GetPlayerChit();
	if ( cs && player ) {
		GameItem* item = player->GetItem();
		// Don't clear the avatar's wallet if a scene is pushed - the avatar
		// may be about to use the wallet!
		if ( item && !item->wallet.IsEmpty() ) {
			if ( !lumosGame->IsScenePushed() && !chitBag->IsScenePushed() ) {
				Wallet w = item->wallet.EmptyWallet();
				cs->ParentChit()->GetItem()->wallet.Add( w );
			}
		}
	}
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
	
#if 0
	// Debug port locations.
	Chit* player = this->GetPlayerChit();
	if ( player ) {
		CompositingShader debug( BLEND_NORMAL );
		debug.SetColor( 1, 1, 1, 0.5f );
		CChitArray arr;
		chitBag->FindBuilding(	IStringConst::vault, ToSector( player->GetSpatialComponent()->GetPosition2DI() ),
								0, LumosChitBag::NEAREST, &arr );
		for( int i=0; i<arr.Size(); ++i ) {
			Rectangle2I p = GET_SUB_COMPONENT( arr[i], SpatialComponent, MapSpatialComponent )->PorchPos();
			Vector3F p0 = { (float)p.min.x, 0.1f, (float)p.min.y };
			Vector3F p1 = { (float)(p.max.x+1), 0.1f, (float)(p.max.y+1) };
			GPUDevice::Instance()->DrawQuad( debug, 0, p0, p1, false );
		}
	}
#endif
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
	if ( wg.Pave() )
		return;		// pavement blocks plants!

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
		MapSpatialComponent* ms = new MapSpatialComponent( worldMap, chitBag );
		ms->SetMapPosition( x, y, 1, 1 );
		chit->Add( ms );

		chit->Add( new HealthComponent( engine ) );
		chit->Add( new ScriptComponent( new PlantScript( this, engine, worldMap, weather, type ), engine, &chitBag->census ));
	}
}

