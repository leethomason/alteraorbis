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

using namespace grinliz;

CoreScript::CoreScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine ) 
	: worldMap( map ), 
	  spawnTick( 10*1000 ), 
	  boundID( 0 ),
	  workQueue( 0 )
{
	workQueue = new WorkQueue( map, chitBag, engine );
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
			team = chit->GetItem()->primaryTeam;
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
		scriptContext->chit->GetRenderComponent()->SetProcedural( IStringConst::main, info );

		return true;
	}
	return false;
}


int CoreScript::DoTick( U32 delta, U32 since )
{
	static const int RADIUS = 4;
	Chit* attached = GetAttached(0);

	if ( spawnTick.Delta( since ) && scriptContext->census->ais < TYPICAL_MONSTERS && !attached ) {
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
				"redMantis",
				"mantis",
				"redMantis",
				"mantis",
				"redMantis",
				"redMantis",
				"redMantis"
			};

			const float greater    = (float)(outland*outland) / (float)(100*256);
			static const float rat = 0.25f;

			const char* spawn = 0;
			float roll = scriptContext->chit->random.Uniform();

			if ( roll < greater ) {
				static const char* GREATER[3] = { "cyclops", "fireCyclops", "shockCyclops" };
				spawn = GREATER[ scriptContext->chit->random.Rand( 3 ) ];
			}
			if (!spawn && (roll < rat) ) {
				spawn = "arachnoid";
			}
			if ( !spawn ) {
				spawn = SPAWN[outland];
			}

			IString ispawn = StringPool::Intern( spawn, true );
			int team = GetTeam( ispawn );
			GLASSERT( team != TEAM_NEUTRAL );
			scriptContext->chitBag->NewMonsterChit( pf, spawn, team );
		}
#endif
	}
	if ( attached && scriptContext->chit->GetSpatialComponent() ) {
		Vector3F pos3 = scriptContext->chit->GetSpatialComponent()->GetPosition();
		scriptContext->engine->particleSystem->EmitPD( "core", pos3, V3F_UP, delta );
	}
	if ( !attached ) {
		// Clear the work queue - chit is gone that controls this.
		workQueue->ClearJobs();

		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( 0, &info );
		scriptContext->chit->GetRenderComponent()->SetProcedural( IStringConst::main, info );
	}
	workQueue->DoTick();
	return attached ? 0 : spawnTick.Next();
}


