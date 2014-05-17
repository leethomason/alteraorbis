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
#include "../script/volcanoscript.h"
#include "../script/plantscript.h"
#include "../script/corescript.h"
#include "../script/worldgen.h"
#include "../script/farmscript.h"
#include "../script/distilleryscript.h"
#include "../script/procedural.h"
#include "../script/guardscript.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

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

#include "../grinliz/glperformance.h"

using namespace grinliz;
using namespace tinyxml2;

Weather* StackedSingleton<Weather>::instance	= 0;
Sim* StackedSingleton<Sim>::instance			= 0;

Sim::Sim( LumosGame* g ) : minuteClock( 60*1000 ), secondClock( 1000 ), volcTimer( 10*1000 )
{
	lumosGame = g;
	spawnEnabled = true;
	Screenport* port = lumosGame->GetScreenportMutable();
	const gamedb::Reader* database = lumosGame->GetDatabase();

	itemDB		= new ItemDB();
	worldMap	= new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	engine		= new Engine( port, database, worldMap );
	weather		= new Weather( MAX_MAP_SIZE, MAX_MAP_SIZE );
	reserveBank = new ReserveBank();
	visitors	= new Visitors();

	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	ChitContext context;
	context.Set( engine, worldMap, g );
	chitBag = new LumosChitBag( context, this );
	chitBag->AddListener(this);

	worldMap->AttachEngine( engine, chitBag );
	worldMap->AttachHistory(chitBag->GetNewsHistory());
	playerID = 0;
	avatarTimer = 0;
	currentVisitor = 0;

	random.SetSeedFromTime();

	DumpModel();
}


Sim::~Sim()
{
	delete visitors;
	delete weather;
	delete reserveBank;
	worldMap->AttachEngine( 0, 0 );
	worldMap->AttachHistory(0);
	chitBag->RemoveListener(this);
	delete chitBag;
	delete engine;
	delete worldMap;
	delete itemDB;
}


int    Sim::AgeI() const { return chitBag->AbsTime() / AGE_IN_MSEC; }
double Sim::AgeD() const { return double(chitBag->AbsTime()) / double(AGE_IN_MSEC); }
float  Sim::AgeF() const { return float(AgeD()); }

void Sim::DumpModel()
{
	GLString path;
	GetSystemPath(GAME_SAVE_DIR,"gamemodel.txt", &path);
	FILE* fp = fopen( path.c_str(), "w" );
	GLASSERT( fp );

	for( int plantStage=2; plantStage<4; ++plantStage ) {
		for( int tech=0; tech<4; ++tech ) {

			// assume field at 50%
			double growTime   = double(FarmScript::GrowFruitTime( plantStage, 10 )) / 1000.0;
			double distilTime = double( DistilleryScript::ElixirTime( tech )) / 1000.0;
			double elixirTime = growTime + distilTime;
			double timePerFruit = elixirTime / double(DistilleryScript::ELIXIR_PER_FRUIT);

			double depleteTime = ai::Needs::DecayTime();

			double idealPopulation = depleteTime / timePerFruit;

			fprintf( fp, "Ideal Population per plant. PlantStage=%d (0-3) Tech=%d (0-%d)\n",
					 plantStage,
					 tech,
					 TECH_MAX-1 );

			fprintf( fp, "    growTime=%.1f distilTime=%.1f totalTime=%.1f\n",
					 growTime, distilTime, elixirTime );
			fprintf( fp, "    ELIXIR_PER_FRUIT=%d timePerFruit=%.1f\n", DistilleryScript::ELIXIR_PER_FRUIT, timePerFruit );
			fprintf( fp, "    depleteTime=%.1f\n", depleteTime );

			if ( plantStage == 3 && tech == 1 ) 
				fprintf( fp, "idealPopulation=%.1f <------ \n\n", idealPopulation );
			else
				fprintf( fp, "idealPopulation=%.1f\n\n", idealPopulation );
		}
	}
	fclose( fp );
}


void Sim::Load( const char* mapDAT, const char* gameDAT )
{
	chitBag->DeleteAll();
	worldMap->Load( mapDAT );

	if ( !gameDAT ) {
		// Fresh start
		CreateRockInOutland();
		CreateCores();
	}
	else {
		//QuickProfile qp( "Sim::Load" );
		GLString path;
		GetSystemPath(GAME_SAVE_DIR, gameDAT, &path);

		FILE* fp = fopen(path.c_str(), "rb");
		GLASSERT( fp );
		if ( fp ) {
			StreamReader reader( fp );
			XarcOpen( &reader, "Sim" );

			XARC_SER( &reader, playerID );
			XARC_SER(&reader, avatarTimer);
			XARC_SER(&reader, GameItem::idPool);

			minuteClock.Serialize( &reader, "minuteClock" );
			secondClock.Serialize( &reader, "secondClock" );
			volcTimer.Serialize( &reader, "volcTimer" );
			itemDB->Serialize( &reader );
			reserveBank->Serialize( &reader );
			visitors->Serialize( &reader );
			engine->camera.Serialize( &reader );
			chitBag->Serialize( &reader );

			XarcClose( &reader );

			fclose( fp );
		}
	}
	if ( chitBag->GetChit( playerID )) {
		Chit* player = chitBag->GetChit( playerID );
		// Mark as player controlled so it reacts as expected to player input.
		// This is the primary avatar, and has some special rules.
		player->SetPlayerControlled( true );
	}
}


void Sim::Save( const char* mapDAT, const char* gameDAT )
{
	worldMap->Save( mapDAT );

	{
		QuickProfile qp( "Sim::SaveXarc" );

		GLString path;
		GetSystemPath(GAME_SAVE_DIR, gameDAT, &path);
		FILE* fp = fopen(path.c_str(), "wb");
		if ( fp ) {
			StreamWriter writer(fp, CURRENT_FILE_VERSION);
			XarcOpen( &writer, "Sim" );
			XARC_SER( &writer, playerID );
			XARC_SER(&writer, avatarTimer);
			XARC_SER(&writer, GameItem::idPool);

			minuteClock.Serialize( &writer, "minuteClock" );
			secondClock.Serialize( &writer, "secondClock" );
			volcTimer.Serialize( &writer, "volcTimer" );
			itemDB->Serialize( &writer );
			reserveBank->Serialize( &writer );
			visitors->Serialize( &writer );
			engine->camera.Serialize( &writer );
			chitBag->Serialize( &writer );

			XarcClose( &writer );
			fclose( fp );
		}
	}
}


void Sim::CreateCores()
{
	int ncores = 0;
	CoreScript* cold = 0;
	CoreScript* hot  = 0;

	for (int j = 0; j < NUM_SECTORS; ++j) {
		for (int i = 0; i < NUM_SECTORS; ++i) {
			Vector2I sector = { i, j };
			CoreScript* cs = CreateCore(sector, TEAM_NEUTRAL);

			if (cs) {
				if (!cold) cold = cs;	// first
				hot = cs;	// last
				++ncores;
			}
		}
	}
	cold->SetDefaultSpawn( StringPool::Intern( "arachnoid" ));	// easy starting point for greater spawns
	hot->SetDefaultSpawn( StringPool::Intern( "arachnoid" ));	// easy starting point for greater spawns
	GLOUTPUT(( "nCores=%d\n", ncores ));
}


void Sim::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_DESTROYED_START) {
		if (chit->GetComponent("CoreScript")) {
			Vector2I pos2i = chit->GetSpatialComponent()->GetPosition2DI();
			Vector2I sector = ToSector(pos2i);
			coreCreateList.Push(sector);

			if (chit->PrimaryTeam() != TEAM_NEUTRAL) {
				NewsEvent news(NewsEvent::DOMAIN_DESTROYED, ToWorld2F(pos2i), chit);
				chitBag->GetNewsHistory()->Add(news);

				// All the buildings become neutral. Some become ruins...
				BuildingFilter filter;
				Rectangle2F bounds = ToWorld(InnerSectorBounds(sector));
				chitBag->QuerySpatialHash(&queryArr, bounds, chit, &filter);
				for (int i = 0; i < queryArr.Size(); ++i) {

					// Make an ornamental copy.
					GameItem* item = queryArr[i]->GetItem();
					if (item && queryArr[i]->GetRenderComponent()) {
						Vector2I pos = queryArr[i]->GetSpatialComponent()->Bounds().min;
						float yrot = queryArr[i]->GetSpatialComponent()->GetYRotation();
						Chit* c = chitBag->NewLawnOrnament(pos, item->Name(), TEAM_NEUTRAL);
						c->GetSpatialComponent()->SetYRotation(yrot);
					}
					queryArr[i]->DeRez();
				}
			}
			if (chit->PrimaryTeam() == TEAM_HOUSE0) {
				Vector2I zero = { 0, 0 };
				chitBag->SetHomeSector(zero);
				if (playerID) {
					Chit* player = GetPlayerChit();
					if (player) player->SetPlayerControlled(false);
				}
				playerID = 0;	// FIXME: flag in 2 places...
			}
		}
	}
}


CoreScript* Sim::CreateCore( const Vector2I& sector, int team)
{
	// Destroy the existing core.
	// Create a new core, attached to the player.
	CoreScript* cs = CoreScript::GetCore(sector);
	if (cs) {
		Chit* core = cs->ParentChit();
		GLASSERT(core);

		// Tell all the AIs the core is going away.
		ChitHasAIComponent filter;
		Rectangle2F b = ToWorld(InnerSectorBounds(sector));
		chitBag->QuerySpatialHash(&queryArr, b, 0, &filter);
		for (int i = 0; i < queryArr.Size(); ++i) {
			queryArr[i]->GetAIComponent()->ClearTaskList();
		}

		//chitBag->QueueDelete(core);
		// QueueDelete is safer, but all kinds of asserts fire (correctly)
		// if 2 cores are in the same place. This may cause an issue
		// if CreateCore is called during the DoTick()
		chitBag->DeleteChit(core);
	}

	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem& coreItem = itemDefDB->Get("core");

	const SectorData* sectorDataArr = worldMap->GetSectorData();
	const SectorData& sd = sectorDataArr[sector.y*NUM_SECTORS+sector.x];
	if (sd.HasCore()) {
		Chit* chit = chitBag->NewBuilding(sd.core, "core", 0);

		// 'in use' instead of blocking.
		MapSpatialComponent* ms = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
		GLASSERT(ms);
		ms->SetMode(GRID_IN_USE);
		CoreScript* cs = new CoreScript();
		chit->Add(cs);

		chit->GetItem()->SetProperName(sd.name);

		if (team != TEAM_NEUTRAL) {
			cs->ParentChit()->GetItem()->primaryTeam = team;
			NewsEvent news(NewsEvent::DOMAIN_CREATED, ToWorld2F(sd.core), chit);
			chitBag->GetNewsHistory()->Add(news);
			if (team == TEAM_HOUSE0) {
				chitBag->SetHomeSector(sector);
			}
			// Make the dwellers defend the core.
			chit->Add(new GuardScript());
		}

		return cs;
	}
	return 0;
}


void Sim::CreateAvatar( const grinliz::Vector2I& pos )
{
	Chit* chit = chitBag->NewDenizen(pos, TEAM_HOUSE0);
	chit->SetPlayerControlled( true );
	playerID = chit->ID();
	chitBag->GetCamera( engine )->SetTrack( playerID );

	GameItem* items[3] = { 0, 0, 0 };
	items[0] = chitBag->AddItem( "shield", chit, engine, 0, 0 );
	items[1] = chitBag->AddItem( "blaster", chit, engine, 0, 0 );
	items[2] = chitBag->AddItem( "ring", chit, engine, 0, 0 );

	// Doesn't work, since QCore doesn't have a chitID.
	// Bigger FIXME issue than thought. Probably should
	// add dieties as actual (indestructable) chits.
	/*
	if (NewsHistory::Instance()) {
		NewsEvent news(NewsEvent::FORGED, pos, shield, chit);
		NewsHistory::Instance()->Add(news);
		shield->keyValues.Set("destroyMsg", NewsEvent::UN_FORGED);
	}
	*/

	chit->GetAIComponent()->EnableDebug( true );
	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );

	// Player speed boost
	float boost = 1.5f / 1.2f;
	chit->GetItem()->keyValues.Set( "speed", DEFAULT_MOVE_SPEED*boost );
	chit->GetRenderComponent()->SetAnimationRate(boost);
	chit->GetItem()->hpRegen = 1.0f;

	// For an avatar, we don't get money, since all the Au goes to the core anyway.
	if (ReserveBank::Instance()) {
		Transfer(&ReserveBank::Instance()->bank, &chit->GetItem()->wallet, chit->GetItem()->wallet);
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
	worldMap->DoTick( delta, chitBag );

	chitBag->DoTick( delta );

	// From the CHIT_DESTROYED_START we have a list of
	// Cores that will be going away...check them here,
	// so that we replace as soon as possible.
	for (int i = 0; i < coreCreateList.Size(); ++i ) {
		Vector2I sector = coreCreateList[i];
		CoreScript* sc = CoreScript::GetCore(sector);
		if (!sc) {
			CreateCore(sector, TEAM_NEUTRAL);
			coreCreateList.SwapRemove(i);
			--i;
		}
	}

	int minuteTick = minuteClock.Delta( delta );
	int secondTick = secondClock.Delta( delta );
	int volcano    = volcTimer.Delta( delta );

	// Age of Fire. Needs lots of volcanoes to seed the world.
	static const int VOLC_RAD = VolcanoScript::MAX_RAD;
	static const int VOLC_DIAM = VOLC_RAD*2+1;
	static const int NUM_VOLC = MAX_MAP_SIZE*MAX_MAP_SIZE / (VOLC_DIAM*VOLC_DIAM);
	int MSEC_TO_VOLC = AGE_IN_MSEC / NUM_VOLC;

	int age = AgeI();

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
				CoreScript* cs = CoreScript::GetCore( sector );
				if ( cs && ( !cs->InUse() || age == 0 )) {
					CreateVolcano( x, y );
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

		if (this->SpawnEnabled()) {
			if (visitorData[currentVisitor].id == 0) {
				Chit* chit = chitBag->NewVisitor(currentVisitor);
				visitorData[currentVisitor].id = chit->ID();
			}
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

	if (cs && !player) {
		// There should be an avatar...
		if (avatarTimer == 0) {
			avatarTimer = 10 * 1000;
		}
		else {
			avatarTimer -= delta;
			if (avatarTimer <= 0) {
				if (chitBag->GetHomeCore()) {
					CreateAvatar(cs->ParentChit()->GetSpatialComponent()->GetPosition2DI());
				}
				avatarTimer = 0;
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


void Sim::UpdateUIElements(const Model* models[], int n)
{	
	for (int i = 0; i < n; ++i) {
		Chit* chit = models[i]->userData;
		if (chit) {
			// Remove it from the uiChits so that we can tell what is no longer used.
			int index = uiChits.Find(chit->ID());
			uiChits.SwapRemove(index);	// safe to -1

			RenderComponent* rc = chit->GetRenderComponent();
			if (rc) {
				rc->PositionIcons(true);
			}
		}
	}
	// Everything remaining in the uiChits is no longer in use:
	for (int i = 0; i < uiChits.Size(); ++i) {
		Chit* chit = chitBag->GetChit(uiChits[i]);
		if (chit && chit->GetRenderComponent()) {
			chit->GetRenderComponent()->PositionIcons(false);	// now off-screen
		}
	}

	// Now move the current list to the UI chits for the next frame:
	// (And remember to use IDs - the Chits can be deleted at any time.)
	uiChits.Clear();
	for (int i = 0; i < n; ++i) {
		Chit* chit = models[i]->userData;
		if (chit) {
			uiChits.Push(chit->ID());
		}
	}
}


void Sim::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime, chitBag->BoltMem(), chitBag->NumBolts(), this );
	
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


void Sim::CreateVolcano( int x, int y )
{
	const SectorData& sd = worldMap->GetSector( x, y );
	if ( sd.ports == 0 ) {
		// no point to volcanoes in the outland
		return;
	}

	Chit* chit = chitBag->NewChit();
	chit->Add( new SpatialComponent() );
	chit->Add( new VolcanoScript());

	chit->GetSpatialComponent()->SetPosition( (float)x+0.5f, 0.0f, (float)y+0.5f );
}


Chit* Sim::CreatePlant( int x, int y, int type )
{
	if ( !worldMap->Bounds().Contains( x, y ) ) {
		return 0;
	}
	const SectorData& sd = worldMap->GetSector( x, y );
	if ( sd.ports == 0 ) {
		// no point to plants in the outland
		return 0;
	}

	// About 50,000 plants seems about right.
	int count = chitBag->census.CountPlants();
	if ( count > worldMap->Bounds().Area() / 20 ) {
		return 0;
	}

	// And space them out a little.
	Random r( x+y*1024 );
	if ( r.Rand(4) == 0 )
		return 0;

	const WorldGrid& wg = worldMap->GetWorldGrid( x, y );
	if ( wg.Pave() || wg.Porch() ) {
		return 0;
	}

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
		MapSpatialComponent* ms = new MapSpatialComponent();
		ms->SetMapPosition( x, y, 1, 1 );
		chit->Add( ms );

		chit->Add( new HealthComponent() );
		chit->Add( new PlantScript( type ));
		return chit;
	}
	return 0;
}


void Sim::UseBuilding()
{
	Chit* player = GetPlayerChit();
	if ( !player ) return;

	Vector2I pos2i = player->GetSpatialComponent()->GetPosition2DI();
	Vector2I sector = ToSector( pos2i );

	Chit* building = chitBag->QueryPorch( pos2i,0 );
	if ( building && building->GetItem() ) {
		IString name = building->GetItem()->IName();
		ItemComponent* ic = player->GetItemComponent();
		CoreScript* cs	= CoreScript::GetCore( sector );
		Chit* core = cs->ParentChit();

		// Messy:
		// Money and crystal are transferred from the avatar to the core. (But 
		// not items.) We need to xfer it *back* before using the factor, market,
		// etc. On the tick, the avatar will restore it to the core.
		ic->GetItem()->wallet.Add( core->GetItem()->wallet.EmptyWallet() );

		if ( cs && ic ) {
			if ( name == IStringConst::vault ) {
				chitBag->PushScene( LumosGame::SCENE_CHARACTER, 
					new CharacterSceneData( ic, building->GetItemComponent(), CharacterSceneData::VAULT, 0 ));
			}
			else if ( name == IStringConst::market ) {
				// The avatar doesn't pay sales tax.
				chitBag->PushScene( LumosGame::SCENE_CHARACTER, 
					new CharacterSceneData( ic, building->GetItemComponent(), CharacterSceneData::MARKET, 0 ));
			}
			else if (name == IStringConst::exchange) {
				chitBag->PushScene(LumosGame::SCENE_CHARACTER,
					new CharacterSceneData(ic, building->GetItemComponent(), CharacterSceneData::EXCHANGE, 0));
			}
			else if ( name == IStringConst::factory ) {
				ForgeSceneData* data = new ForgeSceneData();
				data->tech = int(cs->GetTech());
				data->itemComponent = ic;
				chitBag->PushScene( LumosGame::SCENE_FORGE, data );
			}
		}
	}
}
