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

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"

#include "../script/procedural.h"

static const double TECH_ADDED_BY_VISITOR = 0.2;
static const double TECH_DECAY_0 = 0.0001;
static const double TECH_DECAY_1 = 0.001;
static const double TECH_MAX = 3.5;

using namespace grinliz;

CoreScript::CoreScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine ) 
	: worldMap( map ), 
	  spawnTick( 10*1000 ), 
	  boundID( 0 ),
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
	XARC_SER( xs, boundID );
	XARC_SER( xs, tech );
	XARC_SER( xs, achievedTechLevel );
	XARC_SER( xs, defaultSpawn );
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

	if ( boundID ) {
		scriptContext->chitBag->chitToCoreTable.Add( boundID, scriptContext->chit->ID() );
	}
}


void CoreScript::OnRemove()
{
	if ( boundID ) {
		GLASSERT( scriptContext->chitBag );
		scriptContext->chitBag->chitToCoreTable.Remove( boundID );
	}
}


Chit* CoreScript::GetAttached( bool* standing )
{
	Chit* c = 0;
	if ( boundID ) {
		c = scriptContext->chit->GetChitBag()->GetChit( boundID );
		if ( !c ) {
			boundID = 0;
		}
		if ( c && standing ) {
			if (    !c->GetMoveComponent()
				&& (c->GetSpatialComponent()->GetPosition2DI() == scriptContext->chit->GetSpatialComponent()->GetPosition2DI() ))
			{
				*standing = true;
			}
			else {
				*standing = false;
			}
		}
	}
	return c;
}


bool CoreScript::AttachToCore( Chit* chit )
{
	Chit* bound = GetAttached(0);

	ComponentSet chitset( chit, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	if ( chitset.okay && (!bound || bound == chit) ) {

		LumosChitBag* bag = scriptContext->chit->GetChitBag()->ToLumos();
		bag->chitToCoreTable.Add( chit->ID(), scriptContext->chit->ID() );

		// Set the position of the bound item to the core.
		Vector3F pos = scriptContext->chit->GetSpatialComponent()->GetPosition();
		Vector2F pos2 = { pos.x, pos.z };
		chitset.spatial->SetPosition( pos );
		boundID = chit->ID();

		// Remove the Move component so it doesn't go anywhere or block other motion. But
		// still can be hit, has spatial, etc.
		Component* c = chit->GetMoveComponent();
		chit->Remove( c );
		chit->GetChitBag()->DeferredDelete( c );
		scriptContext->chit->SetTickNeeded();

		// FIXME: check for existing workers
		// FIXME: connect to energy system
		int team = 0;
		if ( chit && chit->GetItem() ) {
			team = chit->PrimaryTeam();
		}

		// FIXME: worker hack until economy is in place.
		CChitArray arr;
		static const char* WORKER[] = { "worker" };
		ItemNameFilter filter( WORKER, 1 );
		chit->GetChitBag()->QuerySpatialHash( &arr, pos2, 10.0f, 0, &filter );

		int count = 2 - arr.Size();
		for( int i=0; i<count; ++i ) {
			scriptContext->chit->GetLumosChitBag()->NewWorkerChit( pos, team );
		}

		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( team, &info );
		scriptContext->chit->GetRenderComponent()->SetProcedural( 0, info );

		return true;
	}
	return false;
}


int CoreScript::DoTick( U32 delta, U32 since )
{
	static const int RADIUS = 4;
	Chit* attached = GetAttached(0);
	bool normalPossible = scriptContext->census->normalMOBs < TYPICAL_MONSTERS;
	bool greaterPossible = scriptContext->census->greaterMOBs < TYPICAL_GREATER;

	tech -= Lerp( TECH_DECAY_0, TECH_DECAY_1, tech/TECH_MAX );
	tech = Clamp( tech, 0.0, TECH_MAX );

	if (    spawnTick.Delta( since ) 
		 && !attached
		 && ( normalPossible || greaterPossible ))
	{
#if 1
		// spawn stuff.
		MapSpatialComponent* ms = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );
		GLASSERT( ms );
		Vector2I pos2i = ms->MapPosition();
		Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

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

			if ( greaterPossible && roll < greater ) {
				static const char* GREATER[4] = { "cyclops", "cyclops", "fireCyclops", "shockCyclops" };
				spawn = GREATER[ scriptContext->chit->random.Rand( 4 ) ];

				Vector2F p2 = { pf.x, pf.z };
				NewsEvent news( NewsEvent::PONY, p2, StringPool::Intern( spawn ), scriptContext->chit->ID() );
				scriptContext->chitBag->AddNews( news );
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
				scriptContext->chitBag->NewMonsterChit( pf, spawn, team );
			}
		}
#endif
	}
	if (    attached 
		 && scriptContext->chit->GetSpatialComponent()
		 && attached->GetSpatialComponent()->GetPosition2DI() == scriptContext->chit->GetSpatialComponent()->GetPosition2DI() ) 
	{
		Vector3F pos3 = scriptContext->chit->GetSpatialComponent()->GetPosition();
		scriptContext->engine->particleSystem->EmitPD( "core", pos3, V3F_UP, delta );
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


void CoreScript::AddTech()
{
	tech += TECH_ADDED_BY_VISITOR;
	tech = Clamp( tech, 0.0, TECH_MAX );

	achievedTechLevel = Max( achievedTechLevel, (int)tech );
}
