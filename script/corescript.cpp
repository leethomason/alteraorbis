#include "corescript.h"

#include "../grinliz/glvector.h"

#include "../engine/engine.h"
#include "../engine/particle.h"
#include "../engine/loosequadtree.h"

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

const char* CoreAchievement::CivTechDescription(int score)
{
	const char* result = "none";
	if (score > 3200)			result = "The mighty empire";
	else if (score > 1600)		result = "The empire";
	else if (score > 800)		result = "The noted domain";
	else if (score > 400)		result = "The domain";
	else if (score > 100)		result = "The outpost";
	else if (score > 50)		result = "The prototype";
	else						result = "The proto test";
	return result;
}

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
	  summonGreater(0),
	  autoRebuild(false)
{
	tech = 0;
	achievement.Clear();
	workQueue = 0;
	pave = 0;
	sector.Zero();
}


CoreScript::~CoreScript()
{
	delete workQueue;
}


void CoreScript::Flag::Serialize(XStream* xs)
{
	XarcOpen(xs, "Flag");
	XARC_SER(xs, pos);
	XarcClose(xs);
}


void CoreScript::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	XARC_SER(xs, tech);
	XARC_SER(xs, summonGreater);
	XARC_SER(xs, autoRebuild);
	XARC_SER(xs, pave);

	XARC_SER(xs, defaultSpawn);

	if (xs->Loading()) {
		int size = 0;
		XarcGet(xs, "citizens.size", size);
		citizens.PushArr(size);
		if (size)
			int debug = 1;
	}
	else {
		XarcSet(xs, "citizens.size", citizens.Size());
	}
	XARC_SER_ARR(xs, citizens.Mem(), citizens.Size());
	XARC_SER_CARRAY(xs, flags);

	spawnTick.Serialize(xs, "spawn");
	scoreTicker.Serialize(xs, "score");
	achievement.Serialize(xs);

	if (!workQueue) {
		workQueue = new WorkQueue();
	}
	workQueue->Serialize(xs);
	EndSerialize(xs);
}


void CoreScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		spawnTick.Randomize(parentChit->ID());
	}
	if ( !workQueue ) {
		workQueue = new WorkQueue();
	}
	Vector2I mapPos = parentChit->GetSpatialComponent()->GetPosition2DI();
	sector = ToSector(mapPos);
	workQueue->InitSector( parentChit, sector );

	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(coreInfoArr[index].coreScript == 0);
	coreInfoArr[index].coreScript = this;

	aiTicker.Randomize(parentChit->random.Rand());

	for (int i = 0; i < flags.Size(); ++i) {
		Model* m = Context()->engine->AllocModel("flag");
		m->SetPos(ToWorld3F(flags[i].pos));
		flags[i].model = m;
	}
}


void CoreScript::OnRemove()
{
	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(coreInfoArr[index].coreScript == this);
	coreInfoArr[index].coreScript = 0;

	delete workQueue;
	workQueue = 0;

	for (int i = 0; i < flags.Size(); ++i) {
		Context()->engine->FreeModel(flags[i].model);
		flags[i].model = 0;
	}

	super::OnRemove();
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
	return Context()->chitBag->GetChit( id );
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
		if ( Context()->chitBag->GetChit( id ) ) {
			++count;
			++i;
		}
		else {
			// Dead and gone.
			citizens.SwapRemove( i );
			// Reset the timer so that there is a little time
			// between a dead citizen and re-spawn
			spawnTick.Reset();
#if 0
			// This is annoying: seeing if cranking down the spawn rate and not
			// destroying the sleep tube achieves success.
			// Also, destroy a sleeptube, so it costs something to replace, and towns can fall.
			SpatialComponent* sc = parentChit->GetSpatialComponent();
			GLASSERT( sc );
			if ( sc ) {
				Vector2F pos2 = sc->GetPosition2D();
				Vector2I sector = ToSector( ToWorld2I( pos2 ));
				Chit* bed = scriptContext->chitBag->FindBuilding( ISC::bed, sector, &pos2, LumosChitBag::RANDOM_NEAR, 0, 0 );
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


void CoreScript::AddFlag(const Vector2I& pos)
{
	Flag f = { pos, 0 };
	if (flags.Find(f) < 0) {
		f.model = Context()->engine->AllocModel("flag");
		f.model->SetPos(ToWorld3F(pos));
		flags.Push(f);
	}
}


void CoreScript::ToggleFlag(const Vector2I& pos)
{
	Flag f = { pos, 0 };
	int i = flags.Find(f);
	if (i >= 0) 
		RemoveFlag(pos);
	else 
		AddFlag(pos);
}


void CoreScript::RemoveFlag(const Vector2I& pos)
{
	Flag f = { pos, 0 };
	int i = flags.Find(f);
	if (i >= 0) {
		Context()->engine->FreeModel(flags[i].model);
		flags.Remove(i);
	}
}


Vector2I CoreScript::GetFlag()
{
	for (int i = 0; i < flags.Size(); ++i) {
		if (tasks.Find(flags[i].pos) < 0) {
			return flags[i].pos;
		}
	}
	Vector2I v = { 0, 0 };
	return v;
}


bool CoreScript::InUse() const
{
	return parentChit->Team() != 0;
}


void CoreScript::UpdateAI()
{
	if (sector.IsZero()) {
		return;
	}
	int index = sector.y*NUM_SECTORS + sector.x;
	CoreInfo* info = &coreInfoArr[index];
	GLASSERT(info->coreScript == this);
}


bool CoreScript::RecruitNeutral()
{
	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	Rectangle2I inner = InnerSectorBounds(sector);

	MOBKeyFilter filter;
	filter.value = ISC::denizen;
	CChitArray arr;
	Context()->chitBag->QuerySpatialHash(&arr, ToWorld(inner), 0, &filter);

	for (int i = 0; i < arr.Size(); ++i) {
		Chit* chit = arr[i];
		if (Team::GetRelationship(chit, parentChit) != RELATE_ENEMY) {
			if (this->IsCitizen(chit)) continue;
			if (!chit->GetItem()) continue;

			if (CoreScript::GetCoreFromTeam(chit->Team()) == 0) {
				// ronin! denizen without a core.
				chit->GetItem()->team = parentChit->Team();
				GLASSERT(chit->GetItem()->Significant());

				NewsEvent news(NewsEvent::ROQUE_DENIZEN_JOINS_TEAM, parentChit->GetSpatialComponent()->GetPosition2D(), chit, 0);
				Context()->chitBag->GetNewsHistory()->Add(news);
			}
		}
	}
	return false;
}


int CoreScript::DoTick(U32 delta)
{
	if ( parentChit->Team() != team ) {
		team = parentChit->Team();
		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( team, &info );
		parentChit->GetRenderComponent()->SetProcedural( 0, info );
	}

	int nScoreTicks = scoreTicker.Delta(delta);
	int nAITicks = aiTicker.Delta(delta);
	int nSpawnTicks = spawnTick.Delta(delta);

	if (InUse()) {
		DoTickInUse(delta, nSpawnTicks);
		UpdateScore(nScoreTicks);
	}
	else {
		DoTickNeutral(delta, nSpawnTicks);
	}
	workQueue->DoTick();

	if (nAITicks) {
		UpdateAI();
	}

	return Min(spawnTick.Next(), aiTicker.Next(), scoreTicker.Next());
}


void CoreScript::DoTickInUse( int delta, int nSpawnTicks )
{
	int team = TeamGroup(parentChit->Team());
	switch (team) {
		case TEAM_HOUSE:
		{
			tech -= Lerp(TECH_DECAY_0, TECH_DECAY_1, tech / double(TECH_MAX));
			int maxTech = MaxTech();
			tech = Clamp(tech, 0.0, double(maxTech) - 0.01);
		}
		break;

		case TEAM_GOB:
		{
			tech = MaxTech();
		}
		break;

		case TEAM_NEUTRAL:
		case TEAM_TROLL:
		tech = 0;
		break;

		default:
		GLASSERT(0);
	}

	MapSpatialComponent* ms = GET_SUB_COMPONENT( parentChit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

	if ( nSpawnTicks ) {
		// Warning: essentially caps the #citizens to the capacity of CChitArray (32)
		// Which just happend to work out with the game design. Citizen limits: 4, 8, 16, 32
		CChitArray chitArr;
		Context()->chitBag->FindBuildingCC( ISC::bed, sector, 0, 0, &chitArr, 0 );
		int nCitizens = this->NumCitizens();

		if ( nCitizens < chitArr.Size() && nCitizens < 32 ) {
			if (!RecruitNeutral()) {
				Context()->chitBag->NewDenizen( pos2i, team );
			}
		}

		if ( (TeamGroup(parentChit->Team()) == TEAM_HOUSE) && (MaxTech() >= TECH_ATTRACTS_GREATER)) {
			summonGreater += spawnTick.Period();
			if (summonGreater > SUMMON_GREATER_TIME) {
				// Find a greater and bring 'em in!
				// This feels a little artificial, and 
				// may need to get revisited as the game
				// grows.
				Context()->chitBag->AddSummoning(sector, LumosChitBag::SUMMON_TECH);
				summonGreater = 0;
			}
		}
	}
}


void CoreScript::DoTickNeutral( int delta, int nSpawnTicks )
{
	int lesser, greater, denizen;
	Context()->chitBag->census.NumByType(&lesser, &greater, &denizen);
	bool greaterPossible = greater < TYPICAL_GREATER;
	bool lesserPossible = lesser < TYPICAL_LESSER;

	MapSpatialComponent* ms = GET_SUB_COMPONENT( parentChit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

	if ( nSpawnTicks && ( lesserPossible || greaterPossible ))
	{
#ifdef SPAWN_MOBS
		if (Context()->chitBag->GetSim() && Context()->chitBag->GetSim()->SpawnEnabled()) {
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
			Context()->chitBag->QuerySpatialHash(&arr, r, 0, &hasAIComponent);
			if (arr.Size() < 2) {
				Vector3F pf = { (float)pos2i.x + 0.5f, 0, (float)pos2i.y + 0.5f };

				if (defaultSpawn.empty()) {
					/*
						What to spawn?
						A core has its "typical spawn": mantis, redManis, trilobyte.
						And an occasional greater spawn: cyclops variants, dragon
						All cores scan spawn trilobyte.
						*/
					static const char* SPAWN[NUM_SECTORS] = {
						"trilobyte",
						"trilobyte",
						"trilobyte",
						"trilobyte",
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

				if (outland > 4 && defaultSpawn == ISC::trilobyte) {
					greater *= 4.f;	// special spots for greaters to spawn.
				}

				float roll = parentChit->random.Uniform();
				bool isGreater = false;

				if (greaterPossible && roll < greater) {
					const grinliz::CDynArray< grinliz::IString >& greater = ItemDefDB::Instance()->GreaterMOBs();
					spawn = greater[parentChit->random.Rand(greater.Size())].c_str();
					isGreater = true;
				}
				if (!spawn && lesserPossible && (roll < rat)) {
					spawn = "trilobyte";
				}
				if (!spawn && lesserPossible) {
					spawn = defaultSpawn.c_str();
				}
				if (spawn) {
					IString ispawn = StringPool::Intern(spawn, true);
					int team = Team::GetTeam(ispawn);
					GLASSERT(team != TEAM_NEUTRAL);
					Chit* mob = Context()->chitBag->NewMonsterChit(pf, spawn, team);
				}
			}
		}
#endif
	}
	// Clear the work queue - chit is gone that controls this.
	delete workQueue;
	workQueue = new WorkQueue();
	workQueue->InitSector(parentChit, parentChit->GetSpatialComponent()->GetSector());
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


int CoreScript::MaxTech()
{
	int team = TeamGroup(parentChit->Team());
	switch (team) {
		case TEAM_HOUSE:
		{
			Vector2I sector = ToSector(parentChit->GetSpatialComponent()->GetPosition2DI());
			CChitArray chitArr;
			Context()->chitBag->FindBuildingCC(ISC::temple, sector, 0, 0, &chitArr, 0);
			return Min(chitArr.Size() + 1, TECH_MAX);	// get one power for core
		}
		break;

		case TEAM_GOB:
		return 1;

		case TEAM_NEUTRAL:
		case TEAM_TROLL:
		return 0;

		default:
		GLASSERT(0);
		break;
	}
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


CoreScript* CoreScript::GetCoreFromTeam(int team)
{
	static int last = 0;
	if (coreInfoArr[last].coreScript && coreInfoArr[last].coreScript->ParentChit()->Team() == team) {
		return coreInfoArr[last].coreScript;
	}

	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		if (coreInfoArr[i].coreScript && coreInfoArr[i].coreScript->ParentChit()->Team() == team) {
			last = i;
			return coreInfoArr[i].coreScript;
		}
	}
	return 0;
}


int CoreScript::GetPave()
{
	if (pave) {
		return pave;
	}

	// Pavement is used as a flag for "this is a road" by the AI.
	// It's important to use the least common pave in a domain
	// so that building isn't interfered with.
	CArray<int, WorldGrid::NUM_PAVE> nPave;
	for (int i = 0; i < WorldGrid::NUM_PAVE; ++i) nPave.Push(0);

	if (pave == 0) {
		Rectangle2I inner = InnerSectorBounds(parentChit->GetSpatialComponent()->GetSector());
		for (Rectangle2IIterator it(inner); !it.Done(); it.Next()) {
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
			nPave[wg.Pave()] += 1;
		}
	}

	nPave[0] = 0;
	if (nPave.Max() == 0) {
		pave = 1 + parentChit->random.Rand(WorldGrid::NUM_PAVE - 1);
	}
	else {
		nPave[0] = SECTOR_SIZE*SECTOR_SIZE;
		nPave.Min(&pave);
	}
	if (pave == 0) {
		pave = 1;
	}
	return pave;
}