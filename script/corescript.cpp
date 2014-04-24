#include "corescript.h"

#include "../grinliz/glvector.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../game/census.h"
#include "../game/lumoschitbag.h"
#include "../game/mapspatialcomponent.h"
#include "../game/worldmap.h"
#include "../game/aicomponent.h"
#include "../game/workqueue.h"
#include "../game/team.h"
#include "../game/lumosmath.h"
#include "../game/gameitem.h"
#include "../game/sim.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/cameracomponent.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"

/*
	See also SectorHerd() for where this gets implemented.
	Temples attrack / repel monsters

	MAX_TECH	Less			Greater
	1: 			repel some		repel
	2: 							repel
	3:			attract near
	4:			attract near	attract
*/
static const double TECH_ADDED_BY_VISITOR = 0.2;
static const double TECH_DECAY_0 = 0.0001;
static const double TECH_DECAY_1 = 0.0016;
static const int SUMMON_GREATER_TIME = 10 * 60 * 1000;

using namespace grinliz;

#define SPAWN_MOBS

CoreInfo CoreScript::coreInfoArr[NUM_SECTORS*NUM_SECTORS];

void CoreAchievement::Serialize(XStream* xs)
{
	XarcOpen(xs, "CoreAchievement");
	XARC_SER(xs, techLevel);
	XARC_SER(xs, gold);
	XARC_SER(xs, population);
	XARC_SER(xs, civTechScore);
	XarcClose(xs);
}

CoreScript::CoreScript() 
	: spawnTick( 20*1000 ), 
	  team( 0 ),
	  workQueue( 0 ),
	  aiTicker(2000),
	  scoreTicker(10*1000),
	  summonGreater(0)
{
	tech = 0;
	achievement.Clear();
	workQueue = 0;
	nElixir = 0;
	sector.Zero();
}


CoreScript::~CoreScript()
{
	delete workQueue;
}


void CoreScript::Init()
{
	spawnTick.Randomize( scriptContext->chit->ID() );
}


void CoreScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, tech );
	XARC_SER( xs, nElixir );
	XARC_SER(xs, summonGreater);

	XARC_SER( xs, defaultSpawn );

	if ( xs->Loading() ) {
		int size=0;
		XarcGet( xs, "citizens.size", size );
		citizens.PushArr( size );
		if ( size ) 
			int debug=1;
	}
	else {
		XarcSet( xs, "citizens.size", citizens.Size() );
	}
	XARC_SER_ARR( xs, citizens.Mem(), citizens.Size() );

	spawnTick.Serialize( xs, "spawn" );
	scoreTicker.Serialize(xs, "score");
	achievement.Serialize(xs);

	if ( !workQueue ) { 
		workQueue = new WorkQueue();
	}
	workQueue->Serialize( xs );
	XarcClose( xs );
}


void CoreScript::OnAdd()
{
	GLASSERT( scriptContext->chit->GetSpatialComponent() );
	GLASSERT( scriptContext->chitBag );
	GLASSERT( scriptContext->engine->GetMap() );

	if ( !workQueue ) {
		workQueue = new WorkQueue();
	}
	Vector2I mapPos = scriptContext->chit->GetSpatialComponent()->GetPosition2DI();
	sector = ToSector(mapPos);
	workQueue->InitSector( scriptContext->chit, sector );

	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(coreInfoArr[index].coreScript == 0);
	coreInfoArr[index].coreScript = this;

	aiTicker.Randomize(scriptContext->chit->random.Rand());
}


void CoreScript::OnRemove()
{
	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(coreInfoArr[index].coreScript == this);
	coreInfoArr[index].coreScript = 0;

	delete workQueue;
	workQueue = 0;
}


void CoreScript::AddCitizen( Chit* chit )
{
	GLASSERT( citizens.Find( chit->ID()) < 0 );
	citizens.Push( chit->ID() );
}


bool CoreScript::IsCitizen( Chit* chit )
{
	return citizens.Find( chit->ID() ) >= 0;
}


bool CoreScript::IsCitizen( int id )
{
	return citizens.Find( id ) >= 0;
}


Chit* CoreScript::CitizenAtIndex( int index )
{
	int id = citizens[index];
	return scriptContext->chitBag->GetChit( id );
}


int CoreScript::FindCitizenIndex( Chit* chit )
{
	GLASSERT( chit );
	int id = chit->ID();
	return citizens.Find( id );
}


int CoreScript::NumCitizens()
{
	int i=0;
	int count=0;
	while ( i < citizens.Size() ) {
		int id = citizens[i];
		if ( scriptContext->chitBag->GetChit( id ) ) {
			++count;
			++i;
		}
		else {
			// Dead and gone.
			citizens.SwapRemove( i );

#if 0
			// This is annoying: seeing if cranking down the spawn rate and not
			// destroying the sleep tube achieves success.
			// Also, destroy a sleeptube, so it costs something to replace, and towns can fall.
			SpatialComponent* sc = scriptContext->chit->GetSpatialComponent();
			GLASSERT( sc );
			if ( sc ) {
				Vector2F pos2 = sc->GetPosition2D();
				Vector2I sector = ToSector( ToWorld2I( pos2 ));
				Chit* bed = scriptContext->chitBag->FindBuilding( IStringConst::bed, sector, &pos2, LumosChitBag::RANDOM_NEAR, 0, 0 );
				if ( bed && bed->GetItem() ) {
					bed->GetItem()->hp = 0;
					bed->SetTickNeeded();
				}
			}
#endif
		}
	}
	return count;
}


bool CoreScript::InUse() const
{
	return PrimaryTeam() != 0;
}


int CoreScript::PrimaryTeam() const
{
	const GameItem* item = scriptContext->chit->GetItem();
	if ( item ) {
		return item->primaryTeam;
	}
	return 0;
}


void CoreScript::UpdateAI()
{
	if (sector.IsZero()) {
		return;
	}
	int index = sector.y*NUM_SECTORS + sector.x;
	CoreInfo* info = &coreInfoArr[index];
	GLASSERT(info->coreScript == this);

	/*
	if (this->InUse()) {
		info->approxTeam = PrimaryTeam();
		CChitArray chitArr;
		scriptContext->chitBag->FindBuildingCC(IStringConst::power, sector, 0, 0, &chitArr, 0);
		info->approxNTemples = chitArr.Size();
	}
	else {
		info->approxNTemples = 0;
		info->approxTeam = 0;
	}
	*/
}



int CoreScript::DoTick( U32 delta )
{
	static const int RADIUS = 4;

	int primaryTeam = 0;
	if ( scriptContext->chit->GetItem() ) {
		primaryTeam = scriptContext->chit->GetItem()->primaryTeam;
	}
	if ( primaryTeam != team ) {
		team = primaryTeam;
		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( team, &info );
		scriptContext->chit->GetRenderComponent()->SetProcedural( 0, info );
	}

	bool normalPossible = scriptContext->census->normalMOBs < TYPICAL_MONSTERS;
	bool greaterPossible = scriptContext->census->greaterMOBs < TYPICAL_GREATER;

	bool inUse = InUse();

	tech -= Lerp( TECH_DECAY_0, TECH_DECAY_1, tech/double(TECH_MAX) );
	int maxTech = MaxTech();
	tech = Clamp( tech, 0.0, double(maxTech)-0.01 );

	MapSpatialComponent* ms = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	int tickd = spawnTick.Delta( delta );

	if ( tickd && inUse ) {
		// FIXME: essentially caps the #citizens to the capacity of CChitArray (32)
		// Which just happend to work out with the game design. Citizen limits: 4, 8, 16, 32
		CChitArray chitArr;
		scriptContext->chitBag->FindBuildingCC( IStringConst::bed, sector, 0, 0, &chitArr, 0 );
		int nCitizens = this->NumCitizens();

		if ( nCitizens < chitArr.Size() && nCitizens < 32 ) {
			Chit* chit = scriptContext->chitBag->NewDenizen( pos2i, team );
		}

		if (maxTech >= TECH_ATTRACTS_GREATER) {
			summonGreater += spawnTick.Period();
			if (summonGreater > SUMMON_GREATER_TIME) {
				// Find a greater and bring 'em in!
				// This feels a little artificial, and 
				// may need to get revisited as the game
				// grows.
				scriptContext->chitBag->AddSummoning(sector, LumosChitBag::SUMMON_TECH);
				summonGreater = 0;
			}
		}
	}
	else if (    tickd 
			  && !inUse
			  && ( normalPossible || greaterPossible ))
	{
#ifdef SPAWN_MOBS
		if (scriptContext->chitBag->GetSim() && scriptContext->chitBag->GetSim()->SpawnEnabled()) {
			// spawn stuff.

			// 0->NUM_SECTORS
			int outland = abs(sector.x - NUM_SECTORS / 2) + abs(sector.y - NUM_SECTORS / 2);
			GLASSERT(NUM_SECTORS == 16);	// else tweak constants 
			outland += Random::Hash8(sector.x + sector.y * 256) % 4;
			outland = Clamp(outland, 0, NUM_SECTORS - 1);

			Rectangle2F r;
			r.Set((float)pos2i.x, (float)(pos2i.y), (float)(pos2i.x + 1), (float)(pos2i.y + 1));
			CChitArray arr;
			ChitHasAIComponent hasAIComponent;
			scriptContext->chitBag->QuerySpatialHash(&arr, r, 0, &hasAIComponent);
			if (arr.Size() < 2) {
				Vector3F pf = { (float)pos2i.x + 0.5f, 0, (float)pos2i.y + 0.5f };

				if (defaultSpawn.empty()) {
					/*
						What to spawn?
						A core has its "typical spawn": mantis, redManis, arachnoid.
						And an occasional greater spawn: cyclops variants, dragon
						All cores scan spawn arachnoids.
						*/
					static const char* SPAWN[NUM_SECTORS] = {
						"arachnoid",
						"arachnoid",
						"arachnoid",
						"arachnoid",
						"mantis",
						"mantis",
						"mantis",
						"redMantis",
						"mantis",
						"troll",
						"mantis",
						"redMantis",
						"mantis",
						"redMantis",
						"troll",
						"redMantis"
					};
					defaultSpawn = StringPool::Intern(SPAWN[outland]);
				}

				float greater = (float)(outland*outland) / (float)(80 * 256);
				static const float rat = 0.25f;
				const char* spawn = 0;

				if (outland > 4 && defaultSpawn == IStringConst::arachnoid) {
					greater *= 4.f;	// special spots for greaters to spawn.
				}

				float roll = scriptContext->chit->random.Uniform();
				bool isGreater = false;

				if (greaterPossible && roll < greater) {
					const grinliz::CDynArray< grinliz::IString >& greater = ItemDefDB::Instance()->GreaterMOBs();
					spawn = greater[scriptContext->chit->random.Rand(greater.Size())].c_str();
					isGreater = true;
				}
				if (!spawn && normalPossible && (roll < rat)) {
					spawn = "arachnoid";
				}
				if (!spawn && normalPossible) {
					spawn = defaultSpawn.c_str();
				}
				if (spawn) {
					IString ispawn = StringPool::Intern(spawn, true);
					int team = GetTeam(ispawn);
					GLASSERT(team != TEAM_NEUTRAL);
					Chit* mob = scriptContext->chitBag->NewMonsterChit(pf, spawn, team);
				}
			}
		}
#endif
	}
	if ( !inUse ) {
		// Clear the work queue - chit is gone that controls this.
		workQueue->ClearJobs();

		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( 0, &info );
		scriptContext->chit->GetRenderComponent()->SetProcedural( 0, info );
	}
	workQueue->DoTick();

	if (aiTicker.Delta(delta)) {
		UpdateAI();
	}

	int nScoreTicks = scoreTicker.Delta(delta);
	if (inUse) {
		UpdateScore(nScoreTicks);
	}

	return Min(spawnTick.Next(), aiTicker.Next(), scoreTicker.Next());
}


void CoreScript::UpdateScore(int n)
{
	if (n) {
		double score = double(citizens.Size()) * sqrt(1.0 + tech);
		score = score * double(n) * double(scoreTicker.Period() / (10.0*1000.0));
		achievement.civTechScore += Min(1, (int)LRint(score));
		// Push this to the GameItem, so it can be recorded in the history + census.
		GameItem* gi = ParentChit()->GetItem();
		if (gi) {
			// Score is a 16 bit quantity...
			int s = achievement.civTechScore;
			if (s > 65535) s = 65535;
			gi->keyValues.Set("score", s);
			gi->UpdateHistory();
		}
	}
	if (ParentChit()->GetItem()) {
		achievement.gold = Max(achievement.gold, ParentChit()->GetItem()->wallet.gold);
		achievement.population = Max(achievement.population, citizens.Size());
	}
}


int CoreScript::MaxTech() const
{
	Vector2I sector = ToSector( scriptContext->chit->GetSpatialComponent()->GetPosition2DI() );
	CChitArray chitArr;
	scriptContext->chitBag->FindBuildingCC( IStringConst::power, sector, 0, 0, &chitArr, 0 );
	return Min( chitArr.Size() + 1, TECH_MAX );	// get one power for core
}


void CoreScript::AddTech()
{
	tech += TECH_ADDED_BY_VISITOR;
	tech = Clamp( tech, 0.0, Min( double(TECH_MAX), double( MaxTech() ) - 0.01 ));

	achievement.techLevel = Max(achievement.techLevel, (int)tech);
}


void CoreScript::AddTask(const grinliz::Vector2I& pos2i)
{
	tasks.Push(pos2i);
}


void CoreScript::RemoveTask(const grinliz::Vector2I& pos2i)
{
	int i = tasks.Find(pos2i);
	//	GLASSERT(i >= 0); with core deletion, this can be tricky...
	// FIXME: should be a safe check. Check with core deletion.
	if (i >= 0){
		tasks.Remove(i);
	}
}


bool CoreScript::HasTask(const grinliz::Vector2I& pos2i)
{
	return tasks.Find(pos2i) >= 0;
}


