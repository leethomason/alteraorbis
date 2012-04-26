#include "debugpathcomponent.h"

#include "../engine/engine.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

using namespace grinliz;

DebugPathComponent::DebugPathComponent( Engine* _engine )
{
	engine = _engine;
	resource = ModelResourceManager::Instance()->GetModelResource( "unitPlateCentered" );
}


DebugPathComponent::~DebugPathComponent()
{
}


void DebugPathComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	model = engine->AllocModel( resource );
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
		if ( render ) {
			model->SetScale( render->RadiusOfBase()*2.0f );
		}
		model->SetFlag( Model::MODEL_NO_SHADOW );
		model->SetTexXForm( 0, 0.5f, 1, 0, 0 );
		model->SetPos( pos );
	}
}

