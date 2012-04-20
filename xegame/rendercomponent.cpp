#include "rendercomponent.h"

#include "../engine/model.h"
#include "../engine/engine.h"

RenderComponent::RenderComponent( Engine* _engine, const char* asset ) : engine( _engine ), model( 0 )
{
	resource = ModelResourceManager::Instance()->GetModelResource( asset );
	GLASSERT( resource );
}


RenderComponent::~RenderComponent()
{
	GLASSERT( model == 0 );
}


void RenderComponent::OnAdd( Chit* )
{
	GLASSERT( model == 0 );
	model = engine->AllocModel( resource );
}


void RenderComponent::OnRemove()
{
	engine->FreeModel( model );
	model = 0;
}