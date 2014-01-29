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

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/cameracomponent.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"

static const double TECH_ADDED_BY_VISITOR = 0.2;
static const double TECH_DECAY_0 = 0.00005;
static const double TECH_DECAY_1 = 0.00020;
static const double TECH_MAX = 4;

using namespace grinliz;

#define SPAWN_MOBS

CoreScript::CoreScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine ) 
	: worldMap( map ), 
	  spawnTick( 10*1000 ), 
	  team( 0 ),
	  workQueue( 0 )
{
	workQueue = new WorkQueue( map, chitBag, engine );
	tech = 0;
	achievedTechLevel = 0;
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
	XARC_SER( xs, achievedTechLevel );
	XARC_SER( xs, defaultSpawn );

	if ( xs->Loading() ) {
		int size=0;
		XarcGet( xs, "citizens.size", size );
		citizens.PushArr( size );
	}
	else {
		XarcSet( xs, "citizens.size", citizens.Size() );
	}
	XARC_SER_ARR( xs, citizens.Mem(), citizens.Size() );

	spawnTick.Serialize( xs, "spawn" );
	workQueue->Serialize( xs );
	XarcClose( xs );
}


void CoreScript::OnAdd()
{
	// Cores are indestructable. They don't get removed.
	GLASSERT( scriptContext->chit->GetSpatialComponent() );
	GLASSERT( scriptContext->chitBag );
	GLASSERT( scriptContext->engine->GetMap() );

	Vector2I mapPos = scriptContext->chit->GetSpatialComponent()->GetPosition2DI();
	Vector2I sector = { mapPos.x/SECTOR_SIZE, mapPos.y/SECTOR_SIZE };
	workQueue->InitSector( sector );
}


void CoreScript::OnRemove()
{
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

	bool attached = InUse();

	tech -= Lerp( TECH_DECAY_0, TECH_DECAY_1, tech/TECH_MAX );
	tech = Clamp( tech, 0.0, double(MaxTech())-0.01 );

	MapSpatialComponent* ms = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );
	GLASSERT( ms );
	Vector2I pos2i = ms->MapPosition();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	int tickd = spawnTick.Delta( delta );

	if ( tickd && attached ) {
		scriptContext->chitBag->FindBuilding( IStringConst::bed, sector, 0, 0, &chitArr, 0 );

		int nCitizens = this->NumCitizens();

		if ( nCitizens < chitArr.Size() ) {
			Chit* chit = scriptContext->chitBag->NewDenizen( pos2i, team );
		}
	}
	else if (    tickd 
			  && !attached
			  && ( normalPossible || greaterPossible ))
	{
#ifdef SPAWN_MOBS
		// spawn stuff.

		// 0->NUM_SECTORS
		int outland = abs( sector.x - NUM_SECTORS/2 ) + abs( sector.y - NUM_SECTORS/2 );
		GLASSERT( NUM_SECTORS == 16 );	// else tweak constants 
		outland += Random::Hash8( sector.x + sector.y*256 ) % 4;
		outland = Clamp( outland, 0, NUM_SECTORS-1 );

		Rectangle2F r;
		r.Set( (float)pos2i.x, (float)(pos2i.y), (float)(pos2i.x+1), (float)(pos2i.y+1) );
		CChitArray arr;
		ChitHasAIComponent hasAIComponent;
		scriptContext->chitBag->QuerySpatialHash( &arr, r, 0, &hasAIComponent );
		if ( arr.Size() < 2 ) {
			Vector3F pf = { (float)pos2i.x+0.5f, 0, (float)pos2i.y+0.5f };

			if ( defaultSpawn.empty() ) {
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
				defaultSpawn = StringPool::Intern( SPAWN[outland] );
			}

			float greater    = (float)(outland*outland) / (float)(80*256);
			static const float rat = 0.25f;
			const char* spawn	   = 0;

			if ( outland > 4 && defaultSpawn == "arachnoid" ) {
				greater *= 4.f;	// special spots for greaters to spawn.
			}

			float roll = scriptContext->chit->random.Uniform();
			bool isGreater = false;

			if ( greaterPossible && roll < greater ) {
				const grinliz::CDynArray< grinliz::IString >& greater = ItemDefDB::Instance()->GreaterMOBs();
				spawn = greater[ scriptContext->chit->random.Rand( greater.Size() ) ].c_str();
				isGreater = true;
			}
			if (!spawn && normalPossible && (roll < rat) ) {
				spawn = "arachnoid";
			}
			if ( !spawn && normalPossible ) {
				spawn = defaultSpawn.c_str();
			}
			if ( spawn ) {
				IString ispawn = StringPool::Intern( spawn, true );
				int team = GetTeam( ispawn );
				GLASSERT( team != TEAM_NEUTRAL );
				Chit* mob = scriptContext->chitBag->NewMonsterChit( pf, spawn, team );

//				if ( isGreater ) {
//					NewsEvent news( NewsEvent::GREATER_MOB_CREATED, ToWorld2F( pf ), mob );
//					NewsHistory::Instance()->Add( news );
//					mob->GetItem()->keyValues.Set( "destroyMsg", "d", NewsEvent::GREATER_MOB_KILLED );
//				}
			}
		}
#endif
	}
	if ( !attached ) {
		// Clear the work queue - chit is gone that controls this.
		workQueue->ClearJobs();

		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( 0, &info );
		scriptContext->chit->GetRenderComponent()->SetProcedural( 0, info );
	}
	workQueue->DoTick();
	return attached ? 0 : spawnTick.Next();
}


int CoreScript::MaxTech() const
{
	Vector2I sector = ToSector( scriptContext->chit->GetSpatialComponent()->GetPosition2DI() );
	scriptContext->chitBag->FindBuilding( IStringConst::power, sector, 0, 0, &chitArr, 0 );
	return Min( chitArr.Size() + 1, 4 );	// get one power for core
}


void CoreScript::AddTech()
{
	tech += TECH_ADDED_BY_VISITOR;
	tech = Clamp( tech, 0.0, Min( TECH_MAX, double( MaxTech() ) - 0.01 ));

	achievedTechLevel = Max( achievedTechLevel, (int)tech );

	// Done by the visitor.
//	RenderComponent* rc = scriptContext->chit->GetRenderComponent();
//	if ( rc ) {
//		rc->AddDeco( "techxfer", STD_DECO );
//	}
}


