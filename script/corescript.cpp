#include "corescript.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glarrayutil.h"

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
#include "../game/worldinfo.h"
#include "../game/reservebank.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/cameracomponent.h"
#include "../xegame/itemcomponent.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"
#include "../script/guardscript.h"

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

CoreInfo CoreScript::coreInfoArr[NUM_SECTORS*NUM_SECTORS];
HashTable<int, int>* CoreScript::teamToCoreInfo = 0;

void CoreScript::Init()
{
	teamToCoreInfo = new HashTable<int, int>();
}

void CoreScript::Free()
{
	delete teamToCoreInfo;
}


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
	  strategicTicker(10*1000),
	  summonGreater(0),
	  autoRebuild(false)
{
	tech = 0;
	achievement.Clear();
	workQueue = 0;
	pave = 0;
	sector.Zero();
	for (int i = 0; i < MAX_SQUADS; ++i) {
		waypointFlags[i] = 0;
	}
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
	XARC_SER_VAL_CARRAY(xs, citizens);
	
	GLASSERT(MAX_SQUADS == 4);
	XARC_SER_VAL_CARRAY(xs, squads[0]);
	XARC_SER_VAL_CARRAY(xs, squads[1]);
	XARC_SER_VAL_CARRAY(xs, squads[2]);
	XARC_SER_VAL_CARRAY(xs, squads[3]);

	XARC_SER_VAL_CARRAY(xs, waypoints[0]);
	XARC_SER_VAL_CARRAY(xs, waypoints[1]);
	XARC_SER_VAL_CARRAY(xs, waypoints[2]);
	XARC_SER_VAL_CARRAY(xs, waypoints[3]);

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
	for (int i = 0; i < MAX_SQUADS; ++i) {
		waypointFlags[i] = 0;
		if (!waypoints[i].Empty()) {
			waypointFlags[i] = Context()->engine->AllocModel("flag");
			waypointFlags[i]->SetPos(ToWorld3F(waypoints[i].Last()));
		}
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
	for (int i = 0; i < MAX_SQUADS; ++i) {
		Context()->engine->FreeModel(waypointFlags[i]);
		waypointFlags[i] = 0;
	}

	super::OnRemove();
}


void CoreScript::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	// Logic split between Sim::OnChitMsg and CoreScript::OnChitMsg
	if (msg.ID() == ChitMsg::CHIT_DESTROYED_START) {
		while (!citizens.Empty()) {
			int citizenID = citizens.Pop();
			Chit* citizen = Context()->chitBag->GetChit(citizenID);
			if (citizen && citizen->GetItem()) {
				// Set to rogue team.
				citizen->GetItem()->SetRogue();
			}
		}
	}
}


void CoreScript::AssignToSquads()
{
	// First, how many do we actually have?
	// Filter out everyone that has gone away.
	for (int i = 0; i < MAX_SQUADS; ++i) {
		GL_ARRAY_FILTER(squads[i], (this->IsCitizen(ele)));
		if (squads[i].Empty()) {
			// Flush out dead squads so they don't have 
			// control flags laying around.
			waypoints[i].Clear();
			Context()->engine->FreeModel(waypointFlags[i]);
		}
	}
	int nSquaddies = 0;
	for (int i = 0; i < MAX_SQUADS; ++i) {
		nSquaddies += squads[i].Size();
	}

	CChitArray recruits;
	int nCitizens = Citizens(&recruits);
	int nExpected = nCitizens - CITIZEN_BASE;
	if (nSquaddies >= nExpected) return;

	// Filter to: not in squad AND not player controlled
	Chit* avatar = Context()->chitBag->GetAvatar();
	GL_ARRAY_FILTER(recruits, (this->SquadID(ele->ID()) < 0 && (ele != avatar) ));
	// Sort the best recruits to the end.
	// FIXME: better if there was a (working) power approximation
	GL_ARRAY_SORT_EXPR(recruits.Mem(), recruits.Size(), 
		(-1 * ele->GetItem()->Traits().Level() * (ele->GetItem()->GetPersonality().Fighting() == Personality::LIKES ? 2 : 1)));

	while (nSquaddies < nExpected) {
		for (int i = 0; i < MAX_SQUADS; ++i) {
			if (squads[i].Size() < SQUAD_SIZE
				&& GetWaypoint(i).IsZero())			// don't add to a squad while it's in use!
			{
				Chit* chit = recruits.Pop();
				squads[i].Push(chit->ID());
				++nSquaddies;
				break;
			}
		}
	}
}


int CoreScript::Squaddies(int id, CChitArray* arr)
{
	GLASSERT(id >= 0 && id < MAX_SQUADS);
	if (arr) arr->Clear();
	GL_ARRAY_FILTER(squads[id], (this->IsCitizen(ele)));
	if (arr) {
		for (int i = 0; i < squads[id].Size(); ++i) {
			Chit* c = Context()->chitBag->GetChit(squads[id][i]);
			if (c && !c->Destroyed()) {
				arr->Push(c);
			}
		}
	}
	return squads[id].Size();
}


bool CoreScript::IsSquaddieOnMission(int chitID)
{
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squads[i].Find(chitID) >= 0) {
			return !waypoints[i].Empty();
		}
	}
	return false;
}


void CoreScript::AddCitizen( Chit* chit )
{
	GLASSERT(ParentChit()->Team());
	GLASSERT(!Team::IsRogue(ParentChit()->Team()));
	GLASSERT( citizens.Find( chit->ID()) < 0 );
	GLASSERT(Team::IsRogue(chit->Team()) || (chit->Team() == ParentChit()->Team()));

	chit->GetItem()->SetTeam(ParentChit()->Team());
	citizens.Push( chit->ID() );
	AssignToSquads();
}


bool CoreScript::IsCitizen( Chit* chit )
{
	return citizens.Find( chit->ID() ) >= 0;
}


bool CoreScript::IsCitizen( int id )
{
	return citizens.Find( id ) >= 0;
}


int CoreScript::SquadID(int id)
{
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squads[i].Find(id) >= 0) {
			return i;
		}
	}
	return -1;
}


Chit* CoreScript::PrimeCitizen()
{
	for (int i = 0; i < citizens.Size(); ++i) {
		int id = citizens[i];
		Chit* chit = Context()->chitBag->GetChit(id);
		if (chit && chit->GetItem()) {
			if (chit->GetItem()->keyValues.Has("prime")) {
				return chit;
			}
		}
	}
	return 0;
}


int CoreScript::Citizens(CChitArray* arr)
{
	int i=0;
	while (i < citizens.Size()) {
		int id = citizens[i];
		Chit* chit = Context()->chitBag->GetChit(id);
		if (chit && !chit->Destroyed()) {
			if (arr) arr->Push(chit);
			++i;
		}
		else {
			// Dead and gone.
			citizens.SwapRemove(i);
			// Reset the timer so that there is a little time
			// between a dead citizen and re-spawn
			spawnTick.Reset();
		}
	}
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
	return citizens.Size();
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

			if (Team::IsRogue(chit->Team())) {
				// ronin! denizen without a core.
				this->AddCitizen( chit );
				GLASSERT(chit->GetItem()->Significant());

				NewsEvent news(NewsEvent::ROQUE_DENIZEN_JOINS_TEAM, parentChit->GetSpatialComponent()->GetPosition2D(), chit, 0);
				Context()->chitBag->GetNewsHistory()->Add(news);
				return true;
			}
		}
	}
	return false;
}


int CoreScript::DoTick(U32 delta)
{
	if ( parentChit->Team() != team ) {
		team = parentChit->Team();
		ProcRenderInfo info;
		AssignProcedural(parentChit->GetItem(), &info);
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

	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squads[i].Empty()) {
			waypoints[i].Clear();
			Context()->engine->FreeModel(waypointFlags[i]);
			waypointFlags[i] = 0;
		}
	}

	if (strategicTicker.Delta(delta)) {
		if (this->InUse() && Context()->chitBag->GetHomeCore() != this) {
			DoStrategicTick();
		}
	}

	return Min(spawnTick.Next(), aiTicker.Next(), scoreTicker.Next());
}


int CoreScript::MaxCitizens(int nTemples)
{
	return CITIZEN_BASE + Min(nTemples, MAX_SQUADS)*SQUAD_SIZE;
}


int CoreScript::NumTemples()
{
	CChitArray chitArr;
	Context()->chitBag->FindBuildingCC( ISC::temple, sector, 0, 0, &chitArr, 0 );
	int nTemples = chitArr.Size();
	return nTemples;
}


int CoreScript::MaxCitizens()
{
	int team = parentChit->Team();

	CChitArray chitArr;
	Context()->chitBag->FindBuildingCC( ISC::bed, sector, 0, 0, &chitArr, 0 );
	int nBeds = chitArr.Size();
	int nTemples = NumTemples();

	int n = CoreScript::MaxCitizens(nTemples);
	return Min(n, nBeds);
}



void CoreScript::DoTickInUse( int delta, int nSpawnTicks )
{
	tech -= Lerp(TECH_DECAY_0, TECH_DECAY_1, tech / double(TECH_MAX));
	int maxTech = MaxTech();
	tech = Clamp(tech, 0.0, double(maxTech) - 0.01);

	MapSpatialComponent* ms = GET_SUB_COMPONENT( parentChit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

	if ( nSpawnTicks ) {
		// Warning: essentially caps the #citizens to the capacity of CChitArray (32)
		// Which just happend to work out with the game design. Citizen limits: 4, 8, 16, 32
		int nCitizens = this->Citizens(0);
		int maxCitizens = this->MaxCitizens();

		if ( nCitizens < maxCitizens ) {
			if (!RecruitNeutral()) {
				Chit* chit = Context()->chitBag->NewDenizen( pos2i, team );
				this->AddCitizen( chit );
			}
		}
	}
}


void CoreScript::DoTickNeutral( int delta, int nSpawnTicks )
{
	int lesser, greater, denizen;
	Context()->chitBag->census.NumByType(&lesser, &greater, &denizen);
	bool lesserPossible = lesser < TYPICAL_LESSER;

	MapSpatialComponent* ms = GET_SUB_COMPONENT( parentChit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

	if ( nSpawnTicks && lesserPossible)
	{
#if SPAWN_MOBS > 0
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

				static const float rat = 0.25f;
				const char* spawn = 0;

				float roll = parentChit->random.Uniform();

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
	if (!workQueue || workQueue->HasJob()) {
		delete workQueue;
		workQueue = new WorkQueue();
		workQueue->InitSector(parentChit, parentChit->GetSpatialComponent()->GetSector());
	}
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
			gi->keyValues.Set(ISC::score, s);
			gi->UpdateHistory();
		}
	}
	if (ParentChit()->GetItem()) {
		achievement.gold = Max(achievement.gold, ParentChit()->GetItem()->wallet.Gold());
		achievement.population = Max(achievement.population, citizens.Size());
	}
}


int CoreScript::MaxTech()
{
	Vector2I sector = ToSector(parentChit->GetSpatialComponent()->GetPosition2DI());
	CChitArray chitArr;
	Context()->chitBag->FindBuildingCC(ISC::temple, sector, 0, 0, &chitArr, 0);
	return Min(chitArr.Size() + 1, TECH_MAX);	// get one power for core
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
	int group = 0, id = 0;
	Team::SplitID(team, &group, &id);
	if (id == 0) 
		return 0;

	int index = 0;
	if (teamToCoreInfo->Query(team, &index)) {
		// Make sure it is current:
		GLASSERT(index >= 0 && index < NUM_SECTORS*NUM_SECTORS);
		CoreScript* cs = coreInfoArr[index].coreScript;
		GLASSERT(cs);
		if (cs && cs->ParentChit()->Team() == team) {
			return cs;
		}
	}

	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		if (coreInfoArr[i].coreScript && coreInfoArr[i].coreScript->ParentChit()->Team() == team) {
			teamToCoreInfo->Add(team, i);
			return coreInfoArr[i].coreScript;
		}
	}
	return 0;
}


CoreScript* CoreScript::CreateCore( const Vector2I& sector, int team, const ChitContext* context)
{
	// Destroy the existing core.
	// Create a new core, attached to the player.
	CoreScript* cs = CoreScript::GetCore(sector);
	if (cs) {
		Chit* core = cs->ParentChit();
		GLASSERT(core);

		CDynArray< Chit* > queryArr;

		// Tell all the AIs the core is going away.
		ChitHasAIComponent filter;
		Rectangle2F b = ToWorld(InnerSectorBounds(sector));
		context->chitBag->QuerySpatialHash(&queryArr, b, 0, &filter);
		for (int i = 0; i < queryArr.Size(); ++i) {
			queryArr[i]->GetAIComponent()->ClearTaskList();
		}

		//context.chitBag->QueueDelete(core);
		// QueueDelete is safer, but all kinds of asserts fire (correctly)
		// if 2 cores are in the same place. This may cause an issue
		// if CreateCore is called during the DoTick()
		context->chitBag->DeleteChit(core);
	}

	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem& coreItem = itemDefDB->Get("core");

	const SectorData& sd = context->worldMap->GetSectorData(sector);
	if (sd.HasCore()) {
		// Assert that the 'team' is correctly formed.
		GLASSERT(team == TEAM_NEUTRAL || team == TEAM_TROLL || team == TEAM_DEITY || Team::ID(team));
		Chit* chit = context->chitBag->NewBuilding(sd.core, "core", team);

		// 'in use' instead of blocking.
		MapSpatialComponent* ms = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
		GLASSERT(ms);
		ms->SetMode(GRID_IN_USE);
		CoreScript* cs = new CoreScript();
		chit->Add(cs);

		chit->GetItem()->SetProperName(sd.name);

		if (team != TEAM_NEUTRAL) {
			NewsEvent news(NewsEvent::DOMAIN_CREATED, ToWorld2F(sd.core), chit);
			context->chitBag->GetNewsHistory()->Add(news);
			// Make the dwellers defend the core.
			chit->Add(new GuardScript());
		}

		return cs;
	}
	return 0;
}



int CoreScript::GetPave()
{
	if (pave) {
		return pave;
	}

	// OLD: Pavement is used as a flag for "this is a road" by the AI.
	// It's important to use the least common pave in a domain
	// so that building isn't interfered with.
	// NOW: Just use least common pave to spread things out.
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


Vector2I CoreScript::GetWaypoint(int squadID)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	static const Vector2I zero = { 0, 0 };
	if (!waypoints[squadID].Empty()) {
		return waypoints[squadID][0];
	}
	return zero;
}


Vector2I CoreScript::GetLastWaypoint(int squadID)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	static const Vector2I zero = { 0, 0 };
	if (!waypoints[squadID].Empty()) {
		return waypoints[squadID].Last();
	}
	return zero;
}


void CoreScript::PopWaypoint(int squadID)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	GLASSERT(waypoints[squadID].Size());
	GLOUTPUT(("Waypoint popped. %d:%d,%d %d remain.\n", squadID, waypoints[squadID][0].x, waypoints[squadID][0].y, waypoints[squadID].Size()-1));
	waypoints[squadID].Remove(0);
	if (waypoints[squadID].Empty()) {
		Context()->engine->FreeModel(waypointFlags[squadID]);
		waypointFlags[squadID] = 0;
	}
}


void CoreScript::SetWaypoints(int squadID, const grinliz::Vector2I& dest)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	if (dest.IsZero()) {
		waypoints[squadID].Clear();
		Context()->engine->FreeModel(waypointFlags[squadID]);
		waypointFlags[squadID] = 0;
		return;
	}

	// Use the first chit to choose the starting location:
	bool startInSameSector = true;
	Vector2I startSector = { 0, 0 };
	CChitArray chitArr;
	Squaddies(squadID, &chitArr);
	if (chitArr.Empty()) return;

	for (int i = 0; i<chitArr.Size(); ++i) {
		Chit* chit = chitArr[i];
		if (startSector.IsZero())
			startSector = chit->GetSpatialComponent()->GetSector();
		else
			if (startSector != chit->GetSpatialComponent()->GetSector())
				startInSameSector = false;
	}

	Vector2I destSector = ToSector(dest);
	waypoints[squadID].Clear();

	// - Current port
	// - grid travel (implies both sector and target port)
	// - dest port (regroup)
	// - destination

	GLOUTPUT(("SetWaypoints: #chits=%d squadID=%d:", chitArr.Size(), squadID));

	if (startSector != destSector) {
		Chit* chit = chitArr[0];
		const SectorData& currentSD = Context()->worldMap->GetSectorData(chit->GetSpatialComponent()->GetSector());
		int currentPort = currentSD.NearestPort(chit->GetSpatialComponent()->GetPosition2D());

		const SectorData& destSD = Context()->worldMap->GetSectorData(destSector);
		int destPort = destSD.NearestPort(ToWorld2F(dest));
		Vector2I p0 = { 0, 0 };

		if (startInSameSector) {
			p0 = currentSD.GetPortLoc(currentPort).Center();	// meet at the STARTING port
		}
		else {
			p0 = destSD.GetPortLoc(destPort).Center();			// meet at the DESTINATION port
		}
		waypoints[squadID].Push(p0);
		GLOUTPUT(("%d,%d [s%x%x]  ", p0.x, p0.y, p0.x / SECTOR_SIZE, p0.y / SECTOR_SIZE));
	}
	GLOUTPUT(("%d,%d [s%x%x]\n", dest.x, dest.y, dest.x/SECTOR_SIZE, dest.y/SECTOR_SIZE));
	waypoints[squadID].Push(dest);

	if (!waypointFlags[squadID]) {
		waypointFlags[squadID] = Context()->engine->AllocModel("flag");
	}
	waypointFlags[squadID]->SetPos(ToWorld3F(waypoints[squadID].Last()));
}


int CoreScript::CorePower()
{
	int power = 0;
	for (int i = 0; i < citizens.Size(); ++i) {
		Chit* chit = Context()->chitBag->GetChit(citizens[i]);
		if (chit && chit->GetItemComponent()) {
			power += chit->GetItemComponent()->PowerRating(true);
		}
	}
	return power;
}


int CoreScript::CoreWealth()
{
	int gold = 0;
	const int* value = ReserveBank::Instance()->CrystalValue();
	const Wallet* wallet = ParentChit()->GetWallet();
	if (wallet) {
		gold += wallet->Gold();
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			gold += value[i] * wallet->Crystal(i);
		}
	}
	return gold;
}


void CoreScript::DoStrategicTick()
{
	// Look around for someone to attack. They should be:
	//	- an enemy FIXME: or neutral
	//	- weaker
	//	- compete for visitors OR have crystal
	//
	// We should:
	//	- have squads ready to go

	// 1. Check for squads available & ready
	// 2. Run through diplomacy list, look for enemies
	// 3. Score on: strength & wealth/visitors
	// 4. Attack

	bool squadReady[MAX_SQUADS] = { false };
	CChitArray squaddies;
	
	// Check for ready squads.
	for (int i = 0; i < MAX_SQUADS; ++i) {
		this->Squaddies(i, &squaddies);
		if (squaddies.Size() < SQUAD_SIZE) 
			continue;

		bool okay = true;
		for (int k = 0; okay && k < squaddies.Size(); ++k) {
			Chit* chit = squaddies[k];
			if (this->IsSquaddieOnMission(chit->ID())) {
				okay = false;
				break;
			}
			else if (chit->GetItem()->HPFraction() < 0.8f) {
				okay = false;
				break;
			}
		}
		squadReady[i] = okay;
	}

	int nReady = 0;
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squadReady[i])
			++nReady;
	}
	if (nReady == 0) return;

	Sim* sim = Context()->chitBag->GetSim();
	GLASSERT(sim);
	if (!sim) return;

	Vector2I sector = ParentChit()->GetSpatialComponent()->GetSector();
	CCoreArray stateArr;
	sim->CalcStrategicRelationships(sector, 3, RELATE_ENEMY, &stateArr);

	int myPower = this->CorePower();
	int myWealth = this->CoreWealth();

	CoreScript* target = 0;
	for (int i = 0; i < stateArr.Size(); ++i) {
		CoreScript* cs = stateArr[i];
		if (cs->NumTemples() == 0)		// Ignore starting out domains so it isn't a complete wasteland out there.
			continue;

		int power = cs->CorePower();
		int wealth   = cs->CoreWealth();

		if ((power < myPower / 2) && (wealth > myWealth || wealth > 400)) {
			// Assuming this is actually so rare that it doesn't matter to select the best.
			target = cs;
			break;
		}
	}

	if (!target) return;

	// Attack!!!
	sim->DeclareWar(target, this);
	bool first = true;
	Vector2F targetCorePos2 = target->ParentChit()->GetSpatialComponent()->GetPosition2D();
	Vector2I targetCorePos = target->ParentChit()->GetSpatialComponent()->GetPosition2DI();
	Vector2I targetSector = ToSector(targetCorePos);

	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (!squadReady[i]) continue;

		Vector2I pos = { 0, 0 };
		pos = targetCorePos;
		if (first) {
			first = false;
		}
		else {
			BuildingWithPorchFilter filter;
			Chit* building = Context()->chitBag->FindBuilding(IString(), targetSector, &targetCorePos2, LumosChitBag::RANDOM_NEAR, 0, &filter);
			if (building) {
				pos = building->GetSpatialComponent()->GetPosition2DI();
			}
		}
		GLASSERT(!pos.IsZero());
		this->SetWaypoints(i, pos);
	}
}
