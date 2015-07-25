#include "sim.h"
#include "../version.h"

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

#include "../ai/domainai.h"
#include "../ai/director.h"

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
#include "circuitsim.h"
#include "gridmovecomponent.h"
#include "physicssims.h"
#include "fluidsim.h"

#include "../xarchive/glstreamer.h"

#include "../grinliz/glperformance.h"
#include "../grinliz/glarrayutil.h"

using namespace grinliz;
using namespace tinyxml2;

Weather* Weather::instance	= 0;

Sim::Sim(LumosGame* g) : minuteClock(60 * 1000), secondClock(1000), volcTimer(10 * 1000), spawnClock(10 * 1000), visitorClock(4*1000)
{
	context.game = g;
	spawnEnabled = 0xff;
	Screenport* port = context.game->GetScreenportMutable();
	const gamedb::Reader* database = context.game->GetDatabase();
	cachedWebAge = VERY_LONG_TICK;

	itemDB				= new ItemDB();
	context.worldMap	= new WorldMap( MAX_MAP_SIZE, MAX_MAP_SIZE );
	context.engine		= new Engine( port, database, context.worldMap );
	weather				= new Weather( MAX_MAP_SIZE, MAX_MAP_SIZE );
	reserveBank			= new ReserveBank();
	teamInfo			= new Team(database);
	visitors			= new Visitors();

	context.engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	context.chitBag		= new LumosChitBag( context, this );
	context.chitBag->AddListener(this);

	context.worldMap->AttachEngine( context.engine, context.chitBag );
	context.worldMap->AttachHistory(context.chitBag->GetNewsHistory());
	avatarTimer = 0;
	currentVisitor = 0;

	context.physicsSims = new PhysicsSims(&context);
	context.worldMap->AttatchPhysics(context.physicsSims);

	random.SetSeedFromTime();
	plantScript = new PlantScript(context.chitBag->Context());

	DumpModel();
}


Sim::~Sim()
{
	context.worldMap->AttatchPhysics(0);
	context.worldMap->AttachEngine( 0, 0 );
	context.worldMap->AttachHistory(0);
	context.chitBag->RemoveListener(this);
	delete context.physicsSims;
	context.physicsSims= 0;
	delete plantScript;
	delete visitors;
	delete weather;
	delete reserveBank;
	delete context.chitBag;
	context.chitBag = 0;
	delete teamInfo;
	delete context.engine;
	context.engine = 0;
	delete context.worldMap;
	context.worldMap = 0;
	delete itemDB;
}


int    Sim::AgeI() const { return context.chitBag->AbsTime() / AGE_IN_MSEC; }
double Sim::AgeD() const { return double(context.chitBag->AbsTime()) / double(AGE_IN_MSEC); }
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


void Sim::Load(const char* mapDAT, const char* gameDAT)
{
	context.chitBag->DeleteAll();
	context.worldMap->Load(mapDAT);

	if (!gameDAT) {
		// Fresh start
		CreateRockInOutland();
		CreateCores();

		Chit* directorChit = context.chitBag->NewChit();
		directorChit->Add(new Director());
	}
	else {
		//QuickProfile qp( "Sim::Load" );
		GLString path;
		GetSystemPath(GAME_SAVE_DIR, gameDAT, &path);

		FILE* fp = fopen(path.c_str(), "rb");
		GLASSERT(fp);
		if (fp) {
			StreamReader reader(fp);
			XarcOpen(&reader, "Sim");

			XARC_SER(&reader, avatarTimer);
			XARC_SER(&reader, GameItem::idPool);

			minuteClock.Serialize(&reader, "minuteClock");
			secondClock.Serialize(&reader, "secondClock");
			volcTimer.Serialize(&reader, "volcTimer");
			spawnClock.Serialize(&reader, "spawnClock");
			visitorClock.Serialize(&reader, "visitorClock");
			itemDB->Serialize(&reader);
			reserveBank->Serialize(&reader);
			teamInfo->Serialize(&reader);
			visitors->Serialize(&reader);
			context.physicsSims->Serialize(&reader);
			context.engine->camera.Serialize(&reader);
			context.chitBag->Serialize(&reader);

			XarcClose(&reader);

			fclose(fp);
		}
	}
}


void Sim::Save(const char* mapDAT, const char* gameDAT)
{
	context.worldMap->Save(mapDAT);

	{
		QuickProfile qp("Sim::SaveXarc");

		GLString path;
		GetSystemPath(GAME_SAVE_DIR, gameDAT, &path);
		FILE* fp = fopen(path.c_str(), "wb");
		if (fp) {
			StreamWriter writer(fp, CURRENT_FILE_VERSION);
			XarcOpen(&writer, "Sim");
			XARC_SER(&writer, avatarTimer);
			XARC_SER(&writer, GameItem::idPool);

			minuteClock.Serialize(&writer, "minuteClock");
			secondClock.Serialize(&writer, "secondClock");
			volcTimer.Serialize(&writer, "volcTimer");
			spawnClock.Serialize(&writer, "spawnClock");
			visitorClock.Serialize(&writer, "visitorClock");
			itemDB->Serialize(&writer);
			reserveBank->Serialize(&writer);
			teamInfo->Serialize(&writer);
			visitors->Serialize(&writer);
			context.physicsSims->Serialize(&writer);
			context.engine->camera.Serialize(&writer);
			context.chitBag->Serialize(&writer);

			XarcClose(&writer);
			fclose(fp);
		}
	}
}


void Sim::CreateCores()
{
	int ncores = 0;

	for (int j = 0; j < NUM_SECTORS; ++j) {
		for (int i = 0; i < NUM_SECTORS; ++i) {
			Vector2I sector = { i, j };

			int team = TEAM_NEUTRAL;
			if (i == NUM_SECTORS / 2 && j == NUM_SECTORS / 2) {
				// Mother Core. Always at the center: spawn point of the Visitors.
				team = DEITY_MOTHER_CORE;
			}
			CoreScript* cs = CoreScript::CreateCore(sector, team, &context);
			if (i == NUM_SECTORS / 2 && j == NUM_SECTORS / 2) {
				cs->ParentChit()->GetItem()->flags |= GameItem::INDESTRUCTABLE;
				HealthComponent* hc = cs->ParentChit()->GetHealthComponent();
				if (hc) {
					cs->ParentChit()->Remove(hc);
					delete hc;
				}
				cs->ParentChit()->Add(new DeityDomainAI());
			}
			if (cs) {
				++ncores;
				GLASSERT(CoreScript::GetCore(sector) == cs);
			}
		}
	}
	GLOUTPUT(( "nCores=%d\n", ncores ));
	AssignDefaultSpawns();
}


void Sim::AssignDefaultSpawns()
{
	int nCores = 0;
	CoreScript** coreScriptArr = CoreScript::GetCoreList(&nCores);

	static const int NSPAWNS = 4;
	static const char* spawns[NSPAWNS] = {
		"trilobyte",
		"mantis",
		"troll",
		"redMantis"
	};
	static const float factor[NSPAWNS] = {
		0.20f,
		0.40f,
		0.20f,
		0.20f
	};
	int count[NSPAWNS] = { 0 };
	for (int i = 0; i < NSPAWNS; ++i) {
		count[i] = int(factor[i] * nCores);
	}
	int subTotal = 0;
	for (int i = 1; i < NSPAWNS; ++i) {
		subTotal += count[i];
	}
	count[0] = nCores - subTotal;

	// A little bit of mixing up.
	CDynArray<const char*> list;
	for (int i = 0; i < NSPAWNS; ++i) {
		for (int k = 0; k < count[i]; ++k) {
			list.Push(spawns[i]);
		}
	}
	static const int FUZZ = 3;
	for (int i = 0; i < list.Size() - FUZZ; ++i) {
		Swap(&list[i], &list[i + random.Rand(FUZZ)]);
	}
	
	list.Reverse();
	for (int i = 0; i < NUM_SECTORS / 2; ++i) {
		Rectangle2I rect;
		rect.Set(NUM_SECTORS / 2 - i - 1, NUM_SECTORS / 2 - i - 1, NUM_SECTORS / 2 + i, NUM_SECTORS / 2 + i);
		for (Rectangle2IEdgeIterator it(rect); !it.Done(); it.Next()) {
			CoreScript* cs = CoreScript::GetCore(it.Pos());
			if (cs) {
				SectorData* sectorData = context.worldMap->GetWorldInfoMutable()->GetSectorDataMutable(it.Pos());
				sectorData->defaultSpawn = StringPool::Intern(list.Pop());
			}
		}
	}
	GLASSERT(list.Empty());
}


void Sim::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
}


void Sim::SpawnDenizens()
{
	// FIXME: should track the relative numbers. Spawn more Kamakiri
	// when there are few Kamakiri domains. But probably need logic
	// to account for both #of Denizens, and Denizens in domains.

	if (!(SpawnEnabled() & SPAWN_DENIZENS))
		return;

	int greater = 0, lesser = 0, denizen = 0;
	context.chitBag->census.NumByType(&lesser, &greater, &denizen);

	if (denizen < TYPICAL_DENIZENS) {
		SectorPort sp;
		for (int i = 0; i < 4; ++i) {
			sp.Zero();
			Vector2I sector = { int(random.Rand(NUM_SECTORS)), int(random.Rand(NUM_SECTORS)) };
			CoreScript* cs = CoreScript::GetCore(sector);
			if (cs && !cs->InUse()) {
				const SectorData& sd = context.worldMap->GetSectorData(sector);
				GLASSERT(sd.ports);
				sp.sector = sector;
				sp.port = sd.RandomPort(&random);
				break;
			}
		}
		if (sp.IsValid()) {
			static const int NSPAWN = 4;
			static const int NUM = 3;

			// Kamakiri tend to be very successful when paired whith green mantis.
			// was: float odds[NUM] = { 100.f, 80.f, 80.f };
			float odds[NUM] = { 100.f, 50.f, 80.f };
			IString iDenizen[NUM] = { ISC::gobman, ISC::kamakiri, ISC::human };

			const Census& census = context.chitBag->census;
			const grinliz::CDynArray<Census::MOBItem>& items = census.MOBItems();
			for (int i = 0; i < NUM; ++i) {
				Census::MOBItem item(iDenizen[i]);
				int index = items.Find(item);
				if (index >= 0) {
					odds[i] /= float(items[index].count);
				}
			}

			int index = random.Select(odds, NUM);

			for (int i = 0; i < NSPAWN; ++i) {
				IString ispawn = iDenizen[index];
				int team = Team::GetTeam(ispawn);
				GLASSERT(team != TEAM_NEUTRAL);

				Vector3F pos = { (float)context.worldMap->Width()*0.5f, 0.0f, (float)context.worldMap->Height()*0.5f };
				Chit* chit = context.chitBag->NewDenizen(ToWorld2I(pos), team);

				int itemType = -1;
				switch (random.Rand(8)) {
					case 1: itemType = ForgeScript::GUN;	break;
					case 2: itemType = ForgeScript::RING;	break;
					case 3: itemType = ForgeScript::SHIELD;	break;
					default: break;
				}
				CoreScript* qCore = CoreScript::GetCoreFromTeam(DEITY_Q);

				if (qCore && itemType >= 0) {
					ForgeScript::ForgeData forgeData;
					forgeData.type = itemType;
					forgeData.subType = ForgeScript::PISTOL;
					forgeData.partsMask = 0;
					forgeData.effectsMask = 0;
					forgeData.tech = 0;
					forgeData.level = 0;
					forgeData.team = DEITY_Q;

					TransactAmt cost;
					GameItem* item = ForgeScript::ForgeRandomItem(forgeData, ReserveBank::Instance()->wallet, &cost, random.Rand());
					if (item) {
						item->wallet.Deposit(ReserveBank::GetWallet(), cost);
						chit->GetItemComponent()->AddToInventory(item);

						// Mark this item as important with a destroyMsg:
						item->SetSignificant(Context()->chitBag->GetNewsHistory(), ToWorld2F(pos), NewsEvent::FORGED, NewsEvent::UN_FORGED, qCore->ParentChit()->GetItem());
					}
				}


				pos.x += 0.2f * float(i);
				chit->SetPosition(pos);

				GridMoveComponent* gmc = new GridMoveComponent();
				chit->Add(gmc);
				gmc->SetDest(sp);
			}
		}
	}
}


void Sim::SpawnGreater()
{
	if (!(SpawnEnabled() & SPAWN_GREATER))
		return;

	int lesser = 0, greater = 0, denizen = 0;
	context.chitBag->census.NumByType(&lesser, &greater, &denizen);
	if (greater < TYPICAL_GREATER) {
		SectorPort sp;
		for (int i = 0; i < 4; ++i) {
			sp.Zero();
			Vector2I sector = RandomInOutland(&random);
			CoreScript* cs = CoreScript::GetCore(sector);
			if (cs && !cs->InUse()) {
				const SectorData& sd = context.worldMap->GetSectorData(sector);
				GLASSERT(sd.ports);
				sp.sector = sector;
				sp.port = sd.RandomPort(&random);
				break;
			}
		}
		if (sp.IsValid()) {

			static const int NUM_GREATER = 3;
			static const char*greaterNames[NUM_GREATER] = { "cyclops", "fireCyclops", "shockCyclops" };
			static const float odds[NUM_GREATER] = { 0.50f, 0.35f, 0.15f };
			int index = random.Select(odds, 3);

			IString ispawn = StringPool::Intern(greaterNames[index], true);
			int team = Team::GetTeam(ispawn);
			GLASSERT(team != TEAM_NEUTRAL);

			Vector3F pos = { (float)context.worldMap->Width()*0.5f, 0.0f, (float)context.worldMap->Height()*0.5f };
			Chit* mob = context.chitBag->NewMonsterChit(pos, ispawn.c_str(), team);
			GridMoveComponent* gmc = new GridMoveComponent();
			mob->Add(gmc);
			gmc->SetDest(sp);
		}
	}
}


void Sim::CreateTruulgaCore()
{
	for (int deityGroup = DEITY_MOTHER_CORE + 1; deityGroup < DEITY_END; ++deityGroup) {
		CoreScript* coreScript = CoreScript::GetCoreFromTeam(deityGroup);

		if (coreScript)	// already built!
			continue;

		// Need a new core for Truulga.
		Vector2I newSector = { random.Rand(NUM_SECTORS), random.Rand(NUM_SECTORS) };
		CoreScript* cs = CoreScript::GetCore(newSector);
		if (cs && cs->ParentChit()->Team() == 0) {
			CoreScript* deity = CoreScript::CreateCore(newSector, deityGroup, &context);
			deity->ParentChit()->Add(new ForgeDomainAI());
			IString name = Team::Instance()->TeamName(deityGroup);
			GLOUTPUT(("Deity %s domain created at %c%d\n", name.safe_str(), 'A' + newSector.x, 1 + newSector.y));
		}
	}
}


void Sim::CreateAvatar( const grinliz::Vector2I& pos )
{
	CoreScript* homeCore = context.chitBag->GetHomeCore();
	GLASSERT(homeCore);
	GLASSERT(!homeCore->PrimeCitizen());

	Chit* chit = context.chitBag->NewDenizen(pos, TEAM_HOUSE);	// real team assigned with AddCitizen
	homeCore->AddCitizen( chit );

	// Help out a newly created Avatar.
	GameItem* items[3] = { 0, 0, 0 };
	items[0] = context.chitBag->AddItem( "shield", chit, context.engine, 0, 0 );
	items[1] = context.chitBag->AddItem( "blaster", chit, context.engine, 0, 0 );
	items[2] = context.chitBag->AddItem( "ring", chit, context.engine, 0, 0 );

	NewsHistory* history = context.chitBag->GetNewsHistory();
	CoreScript* deityCore = CoreScript::GetCoreFromTeam(DEITY_Q);
	if (deityCore) {
		Chit* deity = deityCore->ParentChit();
		for (int i = 0; i < 3; i++) {
			items[i]->SetSignificant(history, ToWorld2F(pos), NewsEvent::FORGED, NewsEvent::UN_FORGED, deity->GetItem());
		}
	}

	chit->SetPosition((float)pos.x + 0.5f, 0, (float)pos.y + 0.5f);
	chit->SetRotation(Quaternion());
	context.chitBag->GetCamera( context.engine )->SetTrack( chit->ID() );

	// Player speed boost
	float boost = 1.5f / 1.2f;
	chit->GetItem()->keyValues.Set( "speed", DEFAULT_MOVE_SPEED*boost );
	chit->GetRenderComponent()->SetAnimationRate(boost);
	chit->GetItem()->hpRegen = 1.0f;
	chit->GetItem()->keyValues.Set("prime", 1);

	// For an avatar, we don't get money, since all the Au goes to the core anyway.
	if (ReserveBank::Instance()) {
		ReserveBank::GetWallet()->Deposit(chit->GetWallet(), *(chit->GetWallet()));
	}
	GLASSERT(chit == homeCore->PrimeCitizen());
}


Texture* Sim::GetMiniMapTexture()
{
	return context.engine->GetMiniMapTexture();
}


void Sim::DoTick( U32 delta, bool useAreaOfInterest )
{
	cachedWebAge += delta;

	context.worldMap->DoTick( delta, context.chitBag );
	plantScript->DoTick(delta);
	context.physicsSims->DoTick(delta);
	Team::Instance()->DoTick(delta);

	if (useAreaOfInterest) {
		Vector3F center = V3F_ZERO;
		context.engine->CameraLookingAt(&center);
		Rectangle3F aoi;
		aoi.min.x = center.x - EL_FAR;
		aoi.min.y = -10.0f;
		aoi.min.z = center.z - EL_FAR;
		aoi.max.x = center.x + EL_FAR;
		aoi.max.y = 100.0f;
		aoi.max.z = center.z + EL_FAR;
		context.chitBag->SetAreaOfInterest(aoi);
	}

	context.chitBag->DoTick( delta );

	CreateTruulgaCore();

	int minuteTick = minuteClock.Delta( delta );
	int volcano    = volcTimer.Delta( delta );

#if SPAWN_MOBS > 0
	if (minuteTick) {
		SpawnGreater();
	}
	if (spawnClock.Delta(delta)) {
		SpawnDenizens();
	}
#endif

	// Age of Fire. Needs lots of volcanoes to seed the world.
	static const int VOLC_RAD = 6;
	static const int VOLC_DIAM = VOLC_RAD*2+1;
	static const int NUM_VOLC = MAX_MAP_SIZE*MAX_MAP_SIZE / (VOLC_DIAM*VOLC_DIAM);
	int MSEC_TO_VOLC = AGE_IN_MSEC / NUM_VOLC;

	int age = AgeI();
	volcTimer.SetPeriod((age >= 1 ? MSEC_TO_VOLC * 4 : MSEC_TO_VOLC) + random.Rand(1000));

	if (volcano) {
		int rock = 0, nSectors = 0;
		for (int y = 0; y < NUM_SECTORS; ++y) {
			for (int x = 0; x < NUM_SECTORS; ++x) {
				Vector2I sector = { x, y };
				FluidSim* fluidSim = context.physicsSims->GetFluidSim(sector);
				if (CoreScript::GetCore(sector) && fluidSim && fluidSim->Settled()) {
					nSectors++;
					rock += fluidSim->NumRocks();
				}
			}
		}
		const static float ROCK_COVERAGE = 0.35f;
		if (nSectors > NUM_SECTORS) {
			float ratio = float(rock) / float(nSectors * SECTOR_SIZE_2);
			if (ratio > ROCK_COVERAGE) {
				volcano = 0;
			}
		}

		while (volcano--) {
			for (int i = 0; i < 5; ++i) {
				int x = random.Rand(context.worldMap->Width());
				int y = random.Rand(context.worldMap->Height());
				if (context.worldMap->IsLand(x, y)) {
					// Don't destroy domains in use. Crazy annoying.
					// ...except deities. They can rebuild, and
					// it keeps from having all flat deity domains.
					Vector2I pos = { x, y };
					Vector2I sector = ToSector(pos);
					CoreScript* cs = CoreScript::GetCore(sector);
					if (!cs || !cs->InUse() || Team::IsDeity(cs->ParentChit()->Team())) {
						CreateVolcano(x, y, random.Rand(4) == 0);
					}
					break;
				}
			}
		}
	}

	if ( visitorClock.Delta(delta) ) {
		VisitorData* visitorData = Visitors::Instance()->visitorData;

		// Check if the visitor is still in the world.
		if ( visitorData[currentVisitor].id ) {
			if ( !context.chitBag->GetChit( visitorData[currentVisitor].id )) {
				visitorData[currentVisitor].id = 0;
			}
		}

		if (this->SpawnEnabled()) {
			if (visitorData[currentVisitor].id == 0) {
				Chit* chit = context.chitBag->NewVisitor(currentVisitor, this->GetCachedWeb());
				visitorData[currentVisitor].id = chit->ID();
			}
		}
		currentVisitor++;
		if ( currentVisitor == Visitors::NUM_VISITORS ) {
			currentVisitor = 0;
		}
	}

	// Cheat! Seed early on, so there is good plant variety.
	int plantCount = context.worldMap->CountPlants();
	if (context.worldMap->CountPlants() < TYPICAL_PLANTS / 2) {
		// And seed lots of plants in the beginning, with extra variation in the early world.
		for (int i = 0; i < 5; ++i) {
			int x = random.Rand(context.worldMap->Width());
			int y = random.Rand(context.worldMap->Height());
			int type = plantCount < TYPICAL_PLANTS / 4 ? random.Rand(NUM_PLANT_TYPES) : -1;
			CreatePlant(x, y, type);
		}
	}
	DoWeatherEffects( delta );

	// Special rule for player controlled chit: give money to the core.
	CoreScript* cs = context.chitBag->GetHomeCore();
	Chit* player = context.chitBag->GetAvatar();
	if ( cs && player ) {
		GameItem* item = player->GetItem();
		// Don't clear the avatar's wallet if a scene is pushed - the avatar
		// may be about to use the wallet!
		if ( item && !item->wallet.IsEmpty() ) {
			if ( !context.game->IsScenePushed() && !context.chitBag->IsScenePushed() ) {
				cs->ParentChit()->GetWallet()->Deposit(&item->wallet, item->wallet);
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
				if (context.chitBag->GetHomeCore()) {
					CreateAvatar(ToWorld2I(cs->ParentChit()->Position()));
				}
				avatarTimer = 0;
			}
		}
	}
}


void Sim::DoWeatherEffects( U32 delta )
{
	Vector3F at;
	context.engine->CameraLookingAt( &at );

	float rain = weather->RainFraction( at.x, at.z );

	if ( rain > 0.5f ) {
		float rainEffect = (rain-0.5f)*2.0f;	// 0-1

		ParticleDef pd = context.engine->particleSystem->GetPD( ISC::splash );
		const float rad = (at - context.engine->camera.PosWC()).Length() * sinf( ToRadian( EL_FOV ));
		const float RAIN_AREA = rad * rad;
		pd.count = (int)(RAIN_AREA * rainEffect * 0.05f );

		/*
			x rain splash	
			x desaturation
			- fog
		*/

		Rectangle3F rainBounds;
		rainBounds.Set( at.x-rad, 0.02f, at.z-rad, at.x+rad, 0.02f, at.z+rad );
		context.worldMap->SetSaturation( 1.0f - 0.50f*rainEffect );

		context.engine->particleSystem->EmitPD( pd, rainBounds, V3F_UP, delta );
	}
	else {
		context.worldMap->SetSaturation( 1 );
	}
	float normalTemp = weather->Temperature( at.x, at.z ) * 2.0f - 1.0f;
	context.engine->SetTemperature( normalTemp );
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
		Chit* chit = context.chitBag->GetChit(uiChits[i]);
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
	context.engine->Draw( deltaTime, context.chitBag->BoltMem(), context.chitBag->NumBolts(), this );
	
#if 0
	// Debug port locations.
	Chit* player = this->GetPlayerChit();
	if ( player ) {
		CompositingShader debug( BLEND_NORMAL );
		debug.SetColor( 1, 1, 1, 0.5f );
		CChitArray arr;
		context.chitBag->FindBuilding(	ISC::vault, ToSector( player->GetPosition2DI() ),
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
	for (int sj = 0; sj < NUM_SECTORS; ++sj) {
		for (int si = 0; si < NUM_SECTORS; ++si) {
			Vector2I sector = { si, sj };
			const SectorData& sd = context.worldMap->GetSectorData(sector);
			if (!sd.HasCore()) {
				for (Rectangle2IIterator it(sd.InnerBounds()); !it.Done(); it.Next()) {
					int i = it.Pos().x;
					int j = it.Pos().y;
					if (context.worldMap->GetWorldGrid(i, j).IsLand()) {
						context.worldMap->SetPlant(i, j, 0, 0);
						context.worldMap->SetRock(i, j, -1, false, 0);
					}
				}
			}
		}
	}
}


void Sim::SetAllRock()
{
	for( int j=0; j<context.worldMap->Height(); ++j ) {
		for( int i=0; i<context.worldMap->Width(); ++i ) {
			context.worldMap->SetRock( i, j, -1, false, 0 );
		}
	}
}


void Sim::CreateVolcano(int x, int y, bool variation)
{
	const SectorData& sd = context.worldMap->GetSectorData(x, y);
	if (sd.ports == 0) {
		// no point to volcanoes in the outland
		return;
	}

	Chit* chit = context.chitBag->NewChit();

	int maxRad = 6;
	EVolcanoType type = EVolcanoType::VOLCANO;
	IString newsString = StringPool::Intern("Volcano");

	if (variation) {
		maxRad = 5 + random.Rand(3);
		float yFactor = float(y) / float(context.worldMap->Height());
		if (yFactor < random.Uniform()) {
			type = EVolcanoType::MOUNTAIN;
			newsString = StringPool::Intern("Volcano:Mnt");
		}
		else {
			type = EVolcanoType::CRATER;
			newsString = StringPool::Intern("Volcano:Crt");
		}
	}

	chit->Add(new VolcanoScript(maxRad, type));

	chit->SetPosition((float)x + 0.5f, 0.0f, (float)y + 0.5f);

	Vector2F pos = ToWorld2F(chit->Position());
	ChitBag::CurrentNews news = { newsString, pos, 0 };
	context.chitBag->PushCurrentNews(news);
}


void Sim::SeedPlants()
{
	Rectangle2I bounds = context.worldMap->Bounds();
	bounds.Outset(-SECTOR_SIZE);

	for (int i = 0; i < TYPICAL_PLANTS; ++i) {
		int x = bounds.min.x + random.Rand(bounds.Width());
		int y = bounds.min.y + random.Rand(bounds.Height());

		int type = random.Rand(NUM_PLANT_TYPES);
		int stage = random.Rand(MAX_PLANT_STAGES);

		CreatePlant(x, y, type, stage);
	}
}


bool Sim::CreatePlant( int x, int y, int type, int stage )
{
	// Pull in plants from edges - don't want to check
	// growth bounds later.
	Rectangle2I bounds = context.worldMap->Bounds();
	bounds.Outset(-8);
	if ( !bounds.Contains( x, y ) ) {
		return false;
	}
	const SectorData& sd = context.worldMap->GetSectorData( x, y );
	if (sd.HasCore() && sd.core.x == x && sd.core.y == y ) {
		// no plants on cores.
		return false;
	}

	// About 50,000 plants seems about right.
	// This doesn't seem to matter - it isn't getting
	// overwhelmed in tests.
//	int count = context.worldMap->CountPlants();
//	if (count > TYPICAL_PLANTS) {
//		return false;
//	}

	Vector2I sector = ToSector(x, y);
	CoreScript* cs = CoreScript::GetCore(sector);
	bool paveBlocks = false;
	if (cs && cs->ParentChit()->Team()) {
		paveBlocks = true;
	}

	const WorldGrid& wg = context.worldMap->GetWorldGrid( x, y );
	if (paveBlocks && wg.Pave()) {
		return false;
	}
	if ( wg.Porch() || wg.IsGrid() || wg.IsPort() || wg.IsWater() ) {
		return false;
	}

	// check for a plant already there.
	if (    wg.IsPassable() 
		 && wg.Plant() == 0
		 && wg.Land() == WorldGrid::LAND 
		 && !context.chitBag->MapGridBlocked(x,y) ) 
	{
		if ( type < 0 ) {
			// Scan for a good type!
			Rectangle2I r;
			r.Set(x, y, x, y);
			r.Outset(3);

			float chance[NUM_PLANT_TYPES];
			for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
				chance[i] = 1.0f;
			}

			for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
				const WorldGrid& wgScan = context.worldMap->GetWorldGrid(it.Pos());
				if (wgScan.Plant()) {
					// Note that in this version, only stage>0 plants
					// change the odds of the plant type. Experimenting
					// with a little more initial plant variety.
					chance[wgScan.Plant() - 1] += (float)(wgScan.PlantStage()*wgScan.PlantStage());
				}
			}
			type = random.Select( chance, NUM_PLANT_TYPES );
		}
		// FIXME: magic plant #s
		if (type == 6 || type == 7) {
			stage = Min(1, stage);
		}
		context.worldMap->SetPave(x, y, 0);
		context.worldMap->SetPlant(x, y, type + 1, stage);
		return true;
	}
	return false;
}


void Sim::UseBuilding()
{
	Chit* player = context.chitBag->GetAvatar();
	if ( !player ) return;

	Vector2I pos2i = ToWorld2I(player->Position());
	Vector2I sector = ToSector( pos2i );

	Chit* building = context.chitBag->QueryPorch( pos2i );
	if ( building && building->GetItem() ) {
		IString name = building->GetItem()->IName();
		ItemComponent* ic = player->GetItemComponent();
		CoreScript* cs	= CoreScript::GetCore( sector );
		Chit* core = cs->ParentChit();

		// Messy:
		// Money and crystal are transferred from the avatar to the core. (But 
		// not items.) We need to xfer it *back* before using the factor, market,
		// etc. On the tick, the avatar will restore it to the core.
		//ic->GetItem()->wallet.Add( core->GetItem()->wallet.EmptyWallet() );
		ic->GetItem()->wallet.Deposit(core->GetWallet(), *(core->GetWallet()));

		if ( cs && ic ) {
			if ( name == ISC::vault ) {
				context.chitBag->PushScene( LumosGame::SCENE_CHARACTER, 
					new CharacterSceneData( ic, building->GetItemComponent(), CharacterSceneData::VAULT, 0 ));
			}
			else if ( name == ISC::market ) {
				// The avatar doesn't pay sales tax.
				context.chitBag->PushScene( LumosGame::SCENE_CHARACTER, 
					new CharacterSceneData( ic, building->GetItemComponent(), CharacterSceneData::MARKET, 0 ));
			}
			else if (name == ISC::exchange) {
				context.chitBag->PushScene(LumosGame::SCENE_CHARACTER,
					new CharacterSceneData(ic, building->GetItemComponent(), CharacterSceneData::EXCHANGE, 0));
			}
			else if ( name == ISC::factory ) {
				ForgeSceneData* data = new ForgeSceneData();
				data->tech = int(cs->GetTech());
				data->itemComponent = ic;
				context.chitBag->PushScene( LumosGame::SCENE_FORGE, data );
			}
			else if (name == ISC::switchOn || name == ISC::switchOff) {
				CircuitSim* circuitSim = context.physicsSims->GetCircuitSim(sector);
				circuitSim->TriggerSwitch(pos2i);
			}
		}
	}
}


const Web& Sim::GetCachedWeb()
{
	if (cachedWebAge > 2000) {
		web.Calc(nullptr);
		cachedWebAge = 0;
	}
	return web;
}


const Web& Sim::CalcWeb()
{
	web.Calc(nullptr);
	cachedWebAge = 0;
	return web;
}


void Sim::CalcStrategicRelationships(const grinliz::Vector2I& sector, int rad, grinliz::CArray<CoreScript*, 32> *stateArr)
{
	stateArr->Clear();

	Rectangle2I mapBounds, bounds;
	mapBounds.Set(1, 1, NUM_SECTORS - 2, NUM_SECTORS - 2);
	bounds.min = bounds.max = sector;
	bounds.Outset(rad);
	bounds.DoIntersection(mapBounds);

	CoreScript* originCore = CoreScript::GetCore(sector);
	if (!originCore) return;

	// Flush the cache:
	CalcWeb();

	for (Rectangle2IIterator it(bounds); !it.Done(); it.Next()) {
		CoreScript* cs = CoreScript::GetCore(it.Pos());
		if (cs
			&& cs != originCore
			&& cs->InUse()
			&& stateArr->HasCap())
		{
			ERelate previousRelate = Team::Instance()->GetRelationship(originCore->ParentChit(), cs->ParentChit());
			Team::Instance()->CalcAttitude(originCore, cs, &web);
			ERelate newRelate = Team::Instance()->GetRelationship(originCore->ParentChit(), cs->ParentChit());

			if (previousRelate != newRelate) {
				int eventID = NewsEvent::ATTITUDE_NEUTRAL;
				if (newRelate == ERelate::FRIEND)		eventID = NewsEvent::ATTITUDE_FRIEND;
				else if (newRelate == ERelate::ENEMY)	eventID = NewsEvent::ATTITUDE_ENEMY;
				NewsEvent newsEvent(eventID,
									ToWorld2F(originCore->ParentChit()->Position()),
									originCore->ParentChit()->GetItemID(),
									cs->ParentChit()->GetItemID());
				context.chitBag->GetNewsHistory()->Add(newsEvent);
			}
			stateArr->Push(cs);
		}
	}
}

