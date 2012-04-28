#include "debugpathcomponent.h"
#include "lumosgame.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"

#include "../engine/engine.h"
#include "../engine/texture.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

using namespace grinliz;

DebugPathComponent::DebugPathComponent( Engine* _engine, WorldMap* _map, LumosGame* _game )
{
	engine = _engine;
	map = _map;
	game = _game;
	resource = ModelResourceManager::Instance()->GetModelResource( "unitPlateCentered" );
}


DebugPathComponent::~DebugPathComponent()
{
}


void DebugPathComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	model = engine->AllocModel( resource );
	model->SetFlag( Model::MODEL_NO_SHADOW );
}


void DebugPathComponent::OnRemove()
{
	Component::OnRemove();
	if ( model ) {
		engine->FreeModel( model );
		model = 0;
	}
}


void DebugPathComponent::DoTick( U32 delta )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( spatial ) {
		Vector3F pos = spatial->GetPosition();
		pos.y = 0.01f;

		RenderComponent* render = parentChit->GetRenderComponent();
		float radius = MAX_BASE_RADIUS;

		if ( render ) {
			model->SetScale( radius*2.0f );
			radius = render->RadiusOfBase();
		}
		model->SetPos( pos );
		Vector4F color = { 0, 1, 0, 1 };
		
		PathMoveComponent* pmc = static_cast<PathMoveComponent*>( parentChit->GetComponent( "PathMoveComponent" ) );

		if ( pmc ) {
			if ( !pmc->IsMoving() )
				color.Set( 0, 0, 0, 1 );
			if ( pmc->BlockForceApplied() )
				color.Set( 1, 1, 0, 1 );
			if ( pmc->IsStuck() )
				color.Set( 1, 0, 0, 1 );
		}

		model->SetColor( color );
	}
}

