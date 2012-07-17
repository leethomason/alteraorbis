#include "rendercomponent.h"
#include "spatialcomponent.h"
#include "inventorycomponent.h"
#include "chit.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../grinliz/glperformance.h"

using namespace grinliz;

RenderComponent::RenderComponent( Engine* _engine, const char* _asset, int _flags ) 
	: engine( _engine ), flags( _flags )
{
	resource[0] = ModelResourceManager::Instance()->GetModelResource( _asset );
	GLASSERT( resource[0] );
	model[0] = 0;
	for( int i=1; i<NUM_MODELS; ++i ) {
		resource[i] = 0;
		model[i] = 0;
	}
}


RenderComponent::~RenderComponent()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		GLASSERT( model[i] == 0 );
	}
}


void RenderComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	for( int i=0; i<NUM_MODELS; ++i ) {
		GLASSERT( model[i] == 0 );
		if ( resource[i] ) {
			model[i] = engine->AllocModel( resource[i] );
			model[i]->userData = parentChit;
			model[i]->SetFlag( flags );
		}
	}
}


void RenderComponent::OnRemove()
{
	Component::OnRemove();
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			engine->FreeModel( model[i] );
			model[i] = 0;
			resource[i] = 0;
		}
	}
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		metaDataName[i].Clear();
	}
}


SpatialComponent* RenderComponent::SyncToSpatial()
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( spatial ) {
		model[0]->SetPos( spatial->GetPosition() );
		model[0]->SetRotation( spatial->GetRotation() );
	}
	return spatial;
}


const char* RenderComponent::GetAnimationName() const
{
	const char* n = "reference";

	MoveComponent* move = parentChit->GetMoveComponent();
	bool isMoving = move && move->IsMoving();
	InventoryComponent* inv = GET_COMPONENT( parentChit, InventoryComponent );
	bool isCarrying = inv && inv->IsCarrying();

	if ( isMoving ) {
		if ( isCarrying ) {
			n = "gunrun";
		}
		else {
			n = "walk";
		}
	}
	else {
		if ( isCarrying ) {
			n = "gunstand";
		}
	}
	return n;
}


void RenderComponent::Attach( const char* metaData, const char* asset )
{
	for( int j=1; j<NUM_MODELS; ++j ) {
		if ( metaDataName[j-1].empty() ) {
			metaDataName[j-1] = metaData;
			GLASSERT( model[j] == 0 );
			resource[j] = ModelResourceManager::Instance()->GetModelResource( asset );
			GLASSERT( resource[j] );

			// If we are already added (model[0] exists) add the attachments.
			if ( model[0] ) {
				model[j] = engine->AllocModel( resource[j] );
				model[j]->userData = parentChit;
				model[j]->SetFlag( flags );
			}
			break;
		}
	}
}


void RenderComponent::Detach( const char* metaData )
{
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( metaDataName[i-1] == metaData ) {
			metaDataName[i-1].Clear();
			if ( model[i] )
				engine->FreeModel( model[i] );
			model[i] = 0;
			resource[i] = 0;
		}
	}
}


void RenderComponent::DoTick( U32 deltaTime )
{
	GRINLIZ_PERFTRACK;

	SpatialComponent* spatial = SyncToSpatial();
	// Animate the primary model.
	if ( spatial && model[0] && model[0]->GetAnimationResource() ) {
		const char* n = this->GetAnimationName();
		model[0]->SetAnimation( n, CROSS_FADE_TIME );
		model[0]->DeltaAnimation( deltaTime, 0 );
	}
	// Position the attachments
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			GLASSERT( !metaDataName[i-1].empty() );
			Matrix4 xform;
			model[0]->CalcMetaData( metaDataName[i-1].c_str(), &xform );
			model[i]->SetTransform( xform );
		}
	}
}


float RenderComponent::RadiusOfBase()
{
	// fixme: cache
	float radius = 0;
	if ( resource ) {
		const Rectangle3F& b = resource[0]->AABB();
		radius = Mean( b.SizeX(), b.SizeZ() );
		radius = Min( radius, MAX_BASE_RADIUS );
	}
	return radius;
}


bool RenderComponent::GetMetaData( const char* name, grinliz::Matrix4* xform )
{
	if ( model ) {
		SyncToSpatial();
		model[0]->CalcMetaData( name, xform );
		return true;
	}
	return false;
}



void RenderComponent::DebugStr( GLString* str )
{
	str->Format( "[Render]=%s ", resource[0]->header.name.c_str() );
}


void RenderComponent::OnChitMsg( Chit* chit, int id, const ChitEvent* event )
{
	if ( chit == parentChit && id == MSG_CHIT_DESTROYED ) {
		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F DOWN = { 0, -1, 0 };
		static const Vector3F RIGHT = { 1, 0, 0 };
		const Vector3F* eyeDir = engine->camera.EyeDir3();
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), UP, eyeDir, 0 );
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), DOWN, eyeDir, 0 );
	}
}
