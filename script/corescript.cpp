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


void CoreScript::Init( const ScriptContext& ctx )
{
	spawnTick.Randomize( ctx.chit->ID() );
}


void CoreScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, boundID );
	spawnTick.Serialize( xs, "spawn" );
	workQueue->Serialize( xs );
	XarcClose( xs );
}


void CoreScript::OnAdd( const ScriptContext& ctx )
{
	// Cores are indestructable. They don't get removed.
	GLASSERT( ctx.chit->GetSpatialComponent() );
	GLASSERT( ctx.chit->GetChitBag() );
	GLASSERT( ctx.engine->GetMap() );

	Vector2I mapPos = ctx.chit->GetSpatialComponent()->GetPosition2DI();
	Vector2I sector = { mapPos.x/SECTOR_SIZE, mapPos.y/SECTOR_SIZE };
	workQueue->InitSector( sector );
}


void CoreScript::OnRemove( const ScriptContext& ctx )
{
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
		scriptContext->chit->GetRenderComponent()->SetProcedural( IStringConst::kmain, info );

		return true;
	}
	return false;
}


int CoreScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	static const int RADIUS = 4;
	Chit* attached = GetAttached(0);

	if ( spawnTick.Delta( since ) && ctx.census->ais < TYPICAL_MONSTERS && !attached ) {
#if 1
		// spawn stuff.
		MapSpatialComponent* ms = GET_SUB_COMPONENT( ctx.chit, SpatialComponent, MapSpatialComponent );
		GLASSERT( ms );
		Vector2I pos = ms->MapPosition();

		Rectangle2F r;
		r.Set( (float)pos.x, (float)(pos.y), (float)(pos.x+1), (float)(pos.y+1) );
		CChitArray arr;
		ChitHasAIComponent hasAIComponent;
		ctx.chit->GetChitBag()->QuerySpatialHash( &arr, r, 0, &hasAIComponent );
		if ( arr.Size() < 2 ) {
			Vector3F pf = { (float)pos.x+0.5f, 0, (float)pos.y+0.5f };
			// FIXME: proper team
			// FIXME safe downcast
			Vector2F p2 = ctx.chit->GetSpatialComponent()->GetPosition2D();

			int team = TEAM_GREEN_MANTIS;
			const char* asset = "mantis";
			if ( ctx.chit->ID() & 1 ) {
				team = TEAM_RED_MANTIS;
				asset = "redMantis";
			}
			((LumosChitBag*)(ctx.chit->GetChitBag()))->NewMonsterChit( pf, asset, team );
		}
#endif
	}
	if ( attached && ctx.chit->GetSpatialComponent() ) {
		Vector3F pos3 = ctx.chit->GetSpatialComponent()->GetPosition();
		ctx.engine->particleSystem->EmitPD( "core", pos3, V3F_UP, delta );
	}
	if ( !attached ) {
		// Clear the work queue - chit is gone that controls this.
		workQueue->ClearJobs();
	}
	workQueue->DoTick();
	return attached ? 0 : spawnTick.Next();
}


