#include "rendercomponent.h"
#include "spatialcomponent.h"
#include "chit.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../grinliz/glperformance.h"

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


void RenderComponent::DoTick( U32 deltaTime )
{
	GRINLIZ_PERFTRACK;

	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( model && spatial ) {
		model->SetPosAndYRotation( spatial->GetPosition(), spatial->GetYRotation() );

		MoveComponent* move = parentChit->GetMoveComponent();
		if ( move ) {
			// Compute the animation:
			// Move: isMoving or not
			// Inventory: has gun
//			model->SetAnimation( move->GetAnimation(), CROSS_FADE_TIME );
			model->DeltaAnimation( deltaTime, 0 );
		}
	}
}


float RenderComponent::RadiusOfBase()
{
	// fixme: cache
	float radius = 0;
	if ( resource ) {
		const Rectangle3F& b = resource->AABB();
		radius = Mean( b.SizeX(), b.SizeZ() );
		radius = Min( radius, MAX_BASE_RADIUS );
	}
	return radius;
}


bool RenderComponent::GetMetaData( const char* name, grinliz::Matrix4* xform )
{
	if ( model ) {
		model->CalcMetaData( name, xform );
		return true;
	}
	return false;
}



void RenderComponent::DebugStr( GLString* str )
{
	str->Format( "[Render]=%s ", resource->header.name.c_str() );
}


void RenderComponent::OnChitMsg( Chit* chit, int id, const ChitEvent* event )
{
	if ( chit == parentChit && id == MSG_CHIT_DESTROYED ) {
		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F DOWN = { 0, -1, 0 };
		static const Vector3F RIGHT = { 1, 0, 0 };
		const Vector3F* eyeDir = engine->camera.EyeDir3();
		engine->particleSystem->EmitPD( "derez", model->AABB().Center(), UP, eyeDir, 0 );
		engine->particleSystem->EmitPD( "derez", model->AABB().Center(), DOWN, eyeDir, 0 );
	}
}
