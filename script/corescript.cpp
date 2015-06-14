#include "corescript.h"

#include "../engine/engine.h"

#include "../game/census.h"
#include "../game/lumoschitbag.h"
#include "../game/mapspatialcomponent.h"
#include "../game/worldmap.h"
#include "../game/aicomponent.h"
#include "../game/workqueue.h"
#include "../game/gameitem.h"
#include "../game/sim.h"
#include "../game/worldinfo.h"
#include "../game/reservebank.h"
#include "../game/healthcomponent.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"
#include "../script/guardscript.h"
#include "../script/flagscript.h"

/*
	See also SectorHerd() for where this gets implemented.
	Temples attract / repel monsters

	MAX_TECH	Less			Greater
	1: 			repel some		repel
	2: 							repel
	3:			attract near
	4:			attract near	attract
*/
static const double TECH_ADDED_BY_VISITOR = 0.2;
static const double TECH_DECAY_0 = 0.0001;
static const double TECH_DECAY_1 = 0.0016;

using namespace grinliz;

CoreInfo CoreScript::coreInfoArr[NUM_SECTORS*NUM_SECTORS];
HashTable<int, int>* CoreScript::teamToCoreInfo = 0;
grinliz::CArray<CoreScript*, NUM_SECTORS*NUM_SECTORS> CoreScript::coreList;


void CoreScript::Init()
{
	teamToCoreInfo = new HashTable<int, int>();	
}

void CoreScript::Free()
{
	delete teamToCoreInfo;
	teamToCoreInfo = 0;
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

CoreScript::CoreScript() :
	  workQueue( 0 ),
	  aiTicker(2000),
	  scoreTicker(10*1000),
	  strategicTicker(10*1000),
	  spawnTick( 20*1000 ), 
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
	XARC_SER(xs, chitID);
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
	Vector2I mapPos = ToWorld2I(parentChit->Position());
	sector = ToSector(mapPos);
	workQueue->InitSector( parentChit, sector );

	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(index);
	GLASSERT(coreInfoArr[index].coreScript == 0);
	coreInfoArr[index].coreScript = this;
	coreList.Push(this);

	aiTicker.Randomize(parentChit->random.Rand());
	strategicTicker.Randomize(parentChit->random.Rand());
}


void CoreScript::OnRemove()
{
	int index = sector.y*NUM_SECTORS + sector.x;
	GLASSERT(coreInfoArr[index].coreScript == this);
	coreInfoArr[index].coreScript = 0;

	int idx = coreList.Find(this);
	GLASSERT(idx >= 0);
	coreList.SwapRemove(idx);

	delete workQueue;
	workQueue = 0;

	super::OnRemove();
}


void CoreScript::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	// Logic split between Sim::OnChitMsg and CoreScript::OnChitMsg
	if (msg.ID() == ChitMsg::CHIT_DESTROYED && (chit == parentChit)) {
		while (!citizens.Empty()) {
			int citizenID = citizens.Pop();
			Chit* citizen = Context()->chitBag->GetChit(citizenID);
			if (citizen && citizen->GetItem()) {
				// Set to rogue team.
				citizen->GetItem()->SetRogue();
			}
		}

		Vector2I pos2i = ToWorld2I(chit->Position());
		Vector2I sector = ToSector(pos2i);

		if (Team::Instance()->IsController(chit->Team())) {
			NewsEvent news(NewsEvent::SUPERTEAM_DELETED, ToWorld2F(pos2i), chit->GetItemID(), 0);
			Context()->chitBag->GetNewsHistory()->Add(news);
		}

		int deleterID = chit->GetItemComponent() ? chit->GetItemComponent()->LastDamageID() : 0;
		Chit* deleter = Context()->chitBag->GetChit(deleterID);
		int superTeam = 0;
		if (deleter
			&& (deleter->Team() == Team::Instance()->SuperTeam(deleter->Team()))
			&& Team::IsDenizen(deleter->Team())
			&& Team::IsDenizen(chit->Team()))
		{
			superTeam = deleter->Team();
		}

		if (chit->Team() != TEAM_NEUTRAL) {
			if (superTeam) {
				LumosChitBag::CreateCoreData data = { sector, true, chit->Team(), deleter ? deleter->Team() : 0 };
				Context()->chitBag->coreCreateList.Push(data);
				NewsEvent news(NewsEvent::DOMAIN_TAKEOVER, ToWorld2F(pos2i), chit->GetItemID(), deleter->GetItemID());
				Context()->chitBag->GetNewsHistory()->Add(news);
			}
			else {
				LumosChitBag::CreateCoreData data = { sector, false, chit->Team(), deleter ? deleter->Team() : 0 };
				Context()->chitBag->coreCreateList.Push(data);
				NewsEvent news(NewsEvent::DOMAIN_DESTROYED, ToWorld2F(pos2i), chit->GetItemID(), deleter ? deleter->GetItemID() : 0);
				Context()->chitBag->GetNewsHistory()->Add(news);
			}
		}
		else {
			// Neutral cores are taken over by wandering over them
			// with enough friend units to have critical mass.
			LumosChitBag::CreateCoreData data = { sector, false, 0, 0 };
			Context()->chitBag->coreCreateList.Push(data);
		}
		Team::Instance()->CoreDestroyed(parentChit->Team());
	}
}


void CoreScript::AssignToSquads()
{
	// First, how many do we actually have?
	// Filter out everyone that has gone away.
	for (int i = 0; i < MAX_SQUADS; ++i) {
		//GL_ARRAY_FILTER(squads[i], (this->IsCitizen(ele)));
		squads[i].Filter(this, [](CoreScript* cs, int id) {
			return cs->IsCitizen(id);
		});
		if (squads[i].Empty()) {
			// Flush out dead squads so they don't have 
			// control flags laying around.
			waypoints[i].Clear();
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

	struct FilterData {
		CoreScript* cs;
		Chit* avatar;;
	};
	FilterData fd = { this, Context()->chitBag->GetAvatar() };

	// Filter to: not in squad AND not player controlled
	recruits.Filter(fd, [](const FilterData& ffd, Chit* chit) {
		return (ffd.cs->SquadID(chit->ID()) < 0 && chit != ffd.avatar);
	});

	// Sort the best recruits to the end.
	// FIXME: better if there was a (working) power approximation
	recruits.Sort([](Chit* a, Chit* b) {
		const GameItem* itemA = a->GetItem();
		const GameItem* itemB = b->GetItem();
		int scoreA = itemA->Traits().Level() * (itemA->GetPersonality().Fighting() == Personality::LIKES ? 2 : 1);
		int scoreB = itemB->Traits().Level() * (itemB->GetPersonality().Fighting() == Personality::LIKES ? 2 : 1);
		
		// Reverse order. Best at end.
		return scoreA > scoreB;
	});

	for (int n = nExpected - nSquaddies; n > 0; --n) {
		for (int i = 0; i < MAX_SQUADS; ++i) {
			if (   (squads[i].Size() < SQUAD_SIZE)
				&& SquadAtRest(i))
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

	squads[id].Filter(this, [](CoreScript* cs, int fid){
		return cs->IsCitizen(fid);
	});
	if (arr) {
		for (int i = 0; i < squads[id].Size(); ++i) {
			Chit* c = Context()->chitBag->GetChit(squads[id][i]);
			if (c) {
				arr->Push(c);
			}
		}
	}
	return squads[id].Size();
}


bool CoreScript::IsSquaddieOnMission(int chitID, int* squadID)
{
	if (!CitizenFilter(chitID)) {
		return false;
	}
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squads[i].Find(chitID) >= 0) {
			if (squadID) {
				*squadID = i;
			}
			return !waypoints[i].Empty();
		}
	}
	return false;
}


void CoreScript::AddCitizen( Chit* chit )
{
	Citizens(0);	// Filters the current array.
	GLASSERT(citizens.Size() < MAX_CITIZENS);
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
	return IsCitizen(chit->ID());
}


bool CoreScript::IsCitizen(int id)
{
	int idx = citizens.Find(id);
	return (idx >= 0) && (CitizenFilter(id) != 0);
}


bool CoreScript::IsCitizenItemID(int id)
{
	for (int i = 0; i < citizens.Size(); ++i) {
		int citizenID = citizens[i];
		Chit* chit = CitizenFilter(citizenID);
		if (chit && chit->GetItemID() == id) {
			return true;
		}
	}
	return false;
}


int CoreScript::SquadID(int chitID)
{
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squads[i].Find(chitID) >= 0) {
			if (IsCitizen(chitID))
				return i;
		}
	}
	return -1;
}


bool CoreScript::SquadAtRest(int squadID)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	if (!waypoints[squadID].Empty()) {
		return false;
	}

	Vector2I sector = ToSector(ParentChit()->Position());
	for (int i = 0; i < squads[squadID].Size(); ++i) {
		int chitID = squads[squadID][i];
		Chit* chit = Context()->chitBag->GetChit(chitID);
		if (chit && ToSector(chit->Position()) != sector) {
			return false;
		}
	}
	return true;
}


Chit* CoreScript::PrimeCitizen()
{
	for (int i = 0; i < citizens.Size(); ++i) {
		int id = citizens[i];
		Chit* chit = CitizenFilter(id);
		if (chit && chit->GetItem()) {
			if (chit->GetItem()->keyValues.Has(ISC::prime)) {
				return chit;
			}
		}
	}
	return 0;
}


Chit* CoreScript::CitizenFilter(int chitID)
{
	Chit* chit = Context()->chitBag->GetChit(chitID);
	if (chit && (chit->Team() == parentChit->Team())) {
		return chit;
	}
	return 0;
}


int CoreScript::Citizens(CChitArray* arr)
{
	int i=0;

	while (i < citizens.Size()) {
		int id = citizens[i];
		Chit* chit = CitizenFilter(id);
		if (chit) {	// check for team change, throw out of citizens.
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


void CoreScript::AddFlag(const Vector2I& _pos)
{
	Vector2I pos = _pos;
	// A little UI fixup: set the flag to a porch
	// if we click on the switch.
	Chit* building = Context()->chitBag->QueryBuilding(IString(), pos, 0);
	if (building && (building->GetItem()->IName() == ISC::switchOn || building->GetItem()->IName() == ISC::switchOff)) {
		MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
		if (msc) {
			pos = msc->PorchPos().min;
		}
	}

	Flag f = { pos, 0 };
	if (flags.Find(f) < 0) {
		Chit* chit = Context()->chitBag->NewChit();
		RenderComponent* rc = new RenderComponent("flag");
		chit->Add(rc);
		ProcRenderInfo info;
		AssignProcedural(ISC::team,
							false,
							ParentChit()->GetItem()->ID(),
							ParentChit()->Team(),
							false,
							0,
							0,
							&info);
		rc->SetProcedural(0, info);
		chit->SetPosition(ToWorld3F(pos));
		f.chitID = chit->ID();
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
		if (flags[i].chitID) {
			Chit* chit = Context()->chitBag->GetChit(flags[i].chitID);
			if (chit) {
				chit->QueueDelete();
			}
			flags[i].chitID = 0;
		}
		flags.Remove(i);
	}
}


Vector2I CoreScript::GetAvailableFlag()
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
	Vector2I sector = ToSector(parentChit->Position());
	Rectangle2I inner = InnerSectorBounds(sector);

	MOBKeyFilter filter;
	filter.value = ISC::denizen;
	CChitArray arr;
	Context()->chitBag->QuerySpatialHash(&arr, ToWorld2F(inner), 0, &filter);

	for (int i = 0; i < arr.Size(); ++i) {
		Chit* chit = arr[i];
		if (Team::Instance()->GetRelationship(chit, parentChit) != ERelate::ENEMY) {
			if (this->IsCitizen(chit)) continue;
			if (!chit->GetItem()) continue;

			if (Team::IsRogue(chit->Team())) {
				// ronin! denizen without a core.
				this->AddCitizen( chit );
				GLASSERT(chit->GetItem()->Significant());

				NewsEvent news(NewsEvent::ROGUE_DENIZEN_JOINS_TEAM, ToWorld2F(chit->Position()), chit->GetItemID(), 0);
				Context()->chitBag->GetNewsHistory()->Add(news);
				return true;
			}
		}
	}
	return false;
}


int CoreScript::DoTick(U32 delta)
{
	int nScoreTicks = scoreTicker.Delta(delta);
	int nAITicks = aiTicker.Delta(delta);

	Citizens(0);	// if someone was deleted, the spawn tick will reset.
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
		}
	}

	if (strategicTicker.Delta(delta)) {
		if (this->InUse() && Context()->chitBag->GetHomeCore() != this) {
			DoStrategicTick();
		}
	}

	RenderComponent* rc = parentChit->GetRenderComponent();
	if (rc) {
		int team = parentChit->Team();
		CStr<40> str = "";
		if (this->InUse() && Team::IsDenizen(team)) {
			IString teamName = Team::Instance()->TeamName(team);
			str.Format("%s", teamName.safe_str());
		}
		rc->SetDecoText(str.safe_str());
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
	Context()->chitBag->FindBuildingCC( ISC::temple, sector, 0, LumosChitBag::EFindMode::NEAREST, &chitArr, 0 );
	int nTemples = chitArr.Size();
	return nTemples;
}


int CoreScript::MaxCitizens()
{
	CChitArray chitArr;
	Context()->chitBag->FindBuildingCC( ISC::bed, sector, 0, LumosChitBag::EFindMode::NEAREST, &chitArr, 0 );
	int nBeds = chitArr.Size();
	int nTemples = NumTemples();
	GLASSERT(nTemples >= 0);

	int n = CoreScript::MaxCitizens(nTemples);
	return Min(n, nBeds);
}



void CoreScript::DoTickInUse(int /*delta*/, int nSpawnTicks)
{
	tech -= Lerp(TECH_DECAY_0, TECH_DECAY_1, tech / double(TECH_MAX));
	int maxTech = MaxTech();
	tech = Clamp(tech, 0.0, double(maxTech) - 0.01);

	MapSpatialComponent* ms = GET_SUB_COMPONENT(parentChit, SpatialComponent, MapSpatialComponent);
	GLASSERT(ms);
	Vector2I pos2i = ms->MapPosition();
	//Vector2I sector = { pos2i.x / SECTOR_SIZE, pos2i.y / SECTOR_SIZE };

	if (nSpawnTicks && Team::IsDenizen(parentChit->Team())) {
		// Warning: essentially caps the #citizens to the capacity of CChitArray (32)
		// Which just happend to work out with the game design. Citizen limits: 4, 8, 16, 32
		int nCitizens = this->Citizens(0);
		int maxCitizens = this->MaxCitizens();

		if (nCitizens < maxCitizens) {
			if (!RecruitNeutral()) {
				Chit* chit = Context()->chitBag->NewDenizen(pos2i, parentChit->Team());
				this->AddCitizen(chit);
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
		int spawnEnabled = Context()->chitBag->GetSim()->SpawnEnabled() & Sim::SPAWN_LESSER;
		if (Context()->chitBag->GetSim() && spawnEnabled) {
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
					Context()->chitBag->NewMonsterChit(pf, spawn, team);
				}
			}
		}
#endif
	}
	// Clear the work queue - chit is gone that controls this.
	if (!workQueue || workQueue->HasJob()) {
		delete workQueue;
		workQueue = new WorkQueue();
		workQueue->InitSector(parentChit, ToSector(parentChit->Position()));
	}
}


float CoreScript::CivTech()
{
	int nCitizens = Citizens(0);
	int nTemples  = NumTemples();
	float citizenScore = float(nCitizens) / float(MAX_CITIZENS);
	float templeScore  = float(nTemples) / float(MAX_SQUADS);
	return (citizenScore + templeScore) * 0.5f;
}


void CoreScript::UpdateScore(int n)
{
	if (n) {
		// Tuned - somewhat sleazily - that basic 4 unit, 0 temple is one point.
		double score = CivTech() * double(n) * double(scoreTicker.Period()) * 0.002;
		achievement.civTechScore += Min(1, int(score));
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
	Vector2I sector = ToSector(parentChit->Position());
	CChitArray chitArr;
	Context()->chitBag->FindBuildingCC(ISC::temple, sector, 0, LumosChitBag::EFindMode::NEAREST, &chitArr, 0);
	return Min(chitArr.Size() + 1, TECH_MAX);	// get one power for core
}


void CoreScript::AddTech()
{
	tech += TECH_ADDED_BY_VISITOR;
	tech = Clamp( tech, 0.0, Min( double(TECH_MAX), double( MaxTech() ) - 0.01 ));
	achievement.techLevel = Max(achievement.techLevel, (int)tech);

	int team = ParentChit()->Team();
	int superTeam = Team::Instance()->SuperTeam(team);
	if (team != superTeam) {
		CoreScript* super = CoreScript::GetCoreFromTeam(superTeam);
		if (super) {
			super->AddTech();
		}
	}
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


CoreScript** CoreScript::GetCoreList(int *n)
{
	*n = coreList.Size();
	return coreList.Mem();
}


CoreScript* CoreScript::GetCoreFromTeam(int team)
{
	if (!teamToCoreInfo) return 0;	// happens on tear down

	int group = 0, id = 0;
	Team::SplitID(team, &group, &id);
	if (id == 0) 
		return 0;

	int index = 0;
	if (teamToCoreInfo->Query(team, &index)) {
		// Make sure it is current:
		GLASSERT(index >= 0 && index < NUM_SECTORS*NUM_SECTORS);
		CoreScript* cs = coreInfoArr[index].coreScript;
//		GLASSERT(cs);	// cores get destroyed at odd times.
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
		Rectangle2F b = ToWorld2F(InnerSectorBounds(sector));
		context->chitBag->QuerySpatialHash(&queryArr, b, 0, &filter);
		for (int i = 0; i < queryArr.Size(); ++i) {
			queryArr[i]->GetAIComponent()->ClearTaskList();
		}

		//context.chitBag->QueueDelete(core);
		// QueueDelete is safer, but all kinds of asserts fire (correctly)
		// if 2 cores are in the same place. This may cause an issue
		// if CreateCore is called during the DoTick()
		// Setting the hp to 0 and then calling DoTick()
		// is a sleazy trick to clean up.
		core->GetItem()->hp = 0;
		if (core->GetHealthComponent()) {
			core->GetHealthComponent()->DoTick(1);
		}
		context->chitBag->DeleteChit(core);
	}

	const SectorData& sd = context->worldMap->GetSectorData(sector);
	if (sd.HasCore()) {
		int group = 0, id = 0;
		Team::SplitID(team, &group, &id);

		// Lots of trouble with this code. Used to assert,
		// but always seemed to be another case. White list
		// groups that *can* take over a core.
		if (team == TEAM_NEUTRAL || team == TEAM_TROLL || group == TEAM_DEITY || Team::IsDenizen(team)) {
			// Okay! take over.
			GLASSERT(!Team::IsDenizen(team) || id);		// make sure rogues got filtered out.
		}
		else {
			team = group = id = 0;
		}
		Chit* chit = context->chitBag->NewBuilding(sd.core, "core", team);

		// 'in use' instead of blocking.
		MapSpatialComponent* ms = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
		GLASSERT(ms);
		ms->SetBlocks(false);
		
		CoreScript* cs = new CoreScript();
		chit->Add(cs);
		GLASSERT(CoreScript::GetCore(ToSector(sd.core)) == cs);

		chit->GetItem()->SetProperName(sd.name);

		if (team != TEAM_NEUTRAL) {
			NewsEvent news(NewsEvent::DOMAIN_CREATED, ToWorld2F(sd.core), chit->GetItemID(), 0);
			context->chitBag->GetNewsHistory()->Add(news);
			// Make the dwellers defend the core.
			chit->Add(new GuardScript());

			// Make all buildings to be this team.
			CDynArray<Chit*> buildings;
			Vector2I buildingSector = ToSector(chit->Position());
			context->chitBag->FindBuilding(IString(), buildingSector, 0, LumosChitBag::EFindMode::NEAREST, &buildings, 0);
			
			for (int i = 0; i < buildings.Size(); ++i) {
				Chit* c = buildings[i];
				if (c->GetItem() && c->GetItem()->IName() != ISC::core) {
					c->GetItem()->SetTeam(team);
				}
			}
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
		Rectangle2I inner = InnerSectorBounds(ToSector(parentChit->Position()));
		for (Rectangle2IIterator it(inner); !it.Done(); it.Next()) {
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
			nPave[wg.Pave()] += 1;
		}
	}

	nPave[0] = 0;
	int maxPave = nPave[nPave.FindMax(0, [](int, int count) { return count; })];

	if (maxPave == 0) {
		pave = 1 + parentChit->random.Rand(WorldGrid::NUM_PAVE - 1);
	}
	else {
		pave = 1 + ArrayFindMax(nPave.Mem()+1, nPave.Size() - 1, 0, [](int, int count) { return -count; });
	}
	GLASSERT(pave > 0 && pave < nPave.Size());
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
}


void CoreScript::SetWaypoints(int squadID, const grinliz::Vector2I& dest)
{
	GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
	if (dest.IsZero() || (waypoints[squadID].Find(dest) >= 0)) {
		waypoints[squadID].Clear();
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
			startSector = ToSector(chit->Position());
		else
			if (startSector != ToSector(chit->Position()))
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
		const SectorData& currentSD = Context()->worldMap->GetSectorData(ToSector(chit->Position()));
		int currentPort = currentSD.NearestPort(ToWorld2F(chit->Position()));

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
		GLOUTPUT(("%d,%d [s%c%d]  ", p0.x, p0.y, 'A' + p0.x / SECTOR_SIZE, 1 + p0.y / SECTOR_SIZE));
	}
	GLOUTPUT(("%d,%d [s%c%d]\n", dest.x, dest.y, 'A' + dest.x/SECTOR_SIZE, 1 + dest.y/SECTOR_SIZE));
	waypoints[squadID].Push(dest);
	NewWaypointChits(squadID);
}


void CoreScript::NewWaypointChits(int id)
{
	for (int i = 0; i < waypoints[id].Size(); ++i) {
		Vector2I v = waypoints[id][i];
		Chit* chit = Context()->chitBag->NewChit();
		chit->Add(new RenderComponent("flag"));
		FlagScript* flagScript = new FlagScript();
		flagScript->Attach(ToSector(parentChit->Position()), id);
		chit->Add(flagScript);
		chit->SetPosition(ToWorld3F(v));
	}
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
	//	- someone we have a negative attitude about (accounts for lots of things)
	//	- weaker overall
	//
	// We should:
	//	- have a reasonably armed squad

	bool squadReady[MAX_SQUADS] = { false };
	CChitArray squaddies;
	
	// Check for ready squads.
	for (int i = 0; i < MAX_SQUADS; ++i) {
		this->Squaddies(i, &squaddies);
		if (squaddies.Size() < SQUAD_SIZE) 
			continue;
		if (!SquadAtRest(i))
			continue;

		// Do we have enough guns? Seems to be
		// the best / easiest metric.

		int nGuns = 0;
		for (int k = 0; k < squaddies.Size(); ++k) {
			Chit* chit = squaddies[k];
			ItemComponent* ic = chit->GetItemComponent();
			if (   chit->GetItem() 
				&& chit->GetItem()->HPFraction() > 0.75f 
				&& ic 
				&& ic->QuerySelectRanged()) 
			{
				nGuns++;
			}
		}
		squadReady[i] = nGuns >= SQUAD_SIZE - 1;
	}

	int nReady = 0;
	for (int i = 0; i < MAX_SQUADS; ++i) {
		if (squadReady[i])
			++nReady;
	}

	Sim* sim = Context()->chitBag->GetSim();
	GLASSERT(sim);
	if (!sim) return;

	Vector2I sector = ToSector(ParentChit()->Position());
	CCoreArray stateArr;
	sim->CalcStrategicRelationships(sector, 3, &stateArr);

	// The strategic relationships need to be calculated, but after that,
	// there's no point in computing further if we don't have a squad to 
	// send into action.
	if (nReady == 0) 
		return;

	int myPower = this->CorePower();

	CoreScript* target = 0;
	for (int i = 0; i < stateArr.Size(); ++i) {
		CoreScript* cs = stateArr[i];
		if (cs->NumTemples() == 0)		// Ignore starting out domains so it isn't a complete wasteland out there.
			continue;

		if (   Team::Instance()->GetRelationship(cs->ParentChit(), this->ParentChit()) == ERelate::ENEMY		// Are we enemies at the diplomatic level
			&& Team::Instance()->Attitude(this, cs) < 0)														// Does 'this' have a negative attitude to the other?
		{
			int power = cs->CorePower();
			if (power < myPower * 0.75f) {
				// Assuming this is actually so rare that it doesn't matter to select the best.
				target = cs;
				break;
			}
		}
	}

	if (!target) return;

	// Attack!!!
	bool first = true;
	Vector2F targetCorePos2 = ToWorld2F(target->ParentChit()->Position());
	Vector2I targetCorePos = ToWorld2I(target->ParentChit()->Position());;
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
			Chit* building = Context()->chitBag->FindBuilding(IString(), targetSector, &targetCorePos2, LumosChitBag::EFindMode::RANDOM_NEAR, 0, &filter);
			if (building) {
				pos = ToWorld2I(building->Position());
			}
		}
		GLASSERT(!pos.IsZero());
		this->SetWaypoints(i, pos);
	}
}
