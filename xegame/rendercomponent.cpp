#include "rendercomponent.h"
#include "spatialcomponent.h"
#include "chit.h"

#include "../engine/model.h"
#include "../engine/engine.h"

using namespace grinliz;

RenderComponent::RenderComponent( Engine* _engine, const char* _asset, int _flags ) 
	: engine( _engine ), model( 0 ), flags( _flags )
{
	resource = ModelResourceManager::Instance()->GetModelResource( _asset );
	GLASSERT( resource );
}


RenderComponent::~RenderComponent()
{
	GLASSERT( model == 0 );
}


void RenderComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	GLASSERT( model == 0 );
	model = engine->AllocModel( resource );
	model->userData = parentChit;
	model->SetFlag( flags );
}


void RenderComponent::OnRemove()
{
	Component::OnRemove();
	engine->FreeModel( model );
	model = 0;
}


void RenderComponent::DoUpdate()
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( model && spatial ) {
		model->SetPos( spatial->GetPosition() );
		model->SetRotation( spatial->GetYRotation(), 1 );
	}
}


float RenderComponent::RadiusOfBase()
{
	float radius = 0;
	if ( resource ) {
		const Rectangle3F& b = resource->AABB();
		radius = Mean( b.SizeX(), b.SizeZ() )*0.5f;
		radius = Min( radius, MAX_BASE_RADIUS );
	}
	return radius;
}

