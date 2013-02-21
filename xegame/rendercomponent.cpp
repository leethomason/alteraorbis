/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rendercomponent.h"
#include "spatialcomponent.h"
#include "inventorycomponent.h"
#include "chit.h"
#include "chitevent.h"
#include "itemcomponent.h"

#include "../engine/animation.h"
#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/particle.h"
#include "../engine/loosequadtree.h"

#include "../grinliz/glperformance.h"
#include "../script/procedural.h"

using namespace grinliz;
using namespace tinyxml2;

RenderComponent::RenderComponent( Engine* _engine, const char* _asset ) 
	: engine( _engine )
{
	resource[0] = 0;
	if ( _asset ) {
		resource[0] = ModelResourceManager::Instance()->GetModelResource( _asset );
		GLASSERT( resource[0] );
	}
	model[0] = 0;
	for( int i=1; i<NUM_MODELS; ++i ) {
		resource[i] = 0;
		model[i] = 0;
	}
	radiusOfBase = 0;
}


RenderComponent::~RenderComponent()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		GLASSERT( model[i] == 0 );
	}
}


void RenderComponent::Serialize( XStream* xs )
{
	BeginSerialize( xs, "RenderComponent" );

	XarcOpen( xs, "resources" );
	for( int i=0; i<NUM_MODELS; ++i ) {
		XarcOpen( xs, "res-mod" );
		IString asset = resource[i] ? resource[i]->IName() : IString();
		XARC_SER( xs, asset );

		if ( xs->Loading() ) {
			if ( !asset.empty() ) {
				resource[i] = ModelResourceManager::Instance()->GetModelResource( asset.c_str() );
				model[i] = engine->AllocModel( resource[i] );
				model[i]->Serialize( xs, engine->GetSpaceTree() );
			}
		}
		else {
			if ( resource[i] ) {
				GLASSERT( model[i] );
				model[i]->Serialize( xs, engine->GetSpaceTree() );
			}
		}
		XarcClose( xs );
	}
	XarcClose( xs );

	XarcOpen( xs, "metaDataNames" );
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		XarcOpen( xs, "meta" );
		XARC_SER_KEY( xs, "meta", metaDataName[i] );
		XarcClose( xs );
	}
	XarcClose( xs );

	EndSerialize( xs );
}


void RenderComponent::Load( const tinyxml2::XMLElement* element )
{
	this->BeginLoad( element, "RenderComponent" );
	
	// Load the resources.
	const XMLElement* resEle = element->FirstChildElement( "resources" );
	GLASSERT( resEle );
	if ( resEle ) {
		int i=0;
		for( const XMLElement* it = resEle->FirstChildElement( "resource" );
			 it && i < NUM_MODELS;
			 i++, it = it->NextSiblingElement( "resource" ) )
		{
			resource[i] = 0;
			const char* asset = it->Attribute( "name" );
			if ( asset ) {
				resource[i] = ModelResourceManager::Instance()->GetModelResource( asset );
			}
		}
	}

	// And now the models.
	const XMLElement* modelEle = element->FirstChildElement( "models" );
	GLASSERT( modelEle );
	if ( modelEle ) {
		int i=0;
		for ( const XMLElement* it = modelEle->FirstChildElement( "Model" );
			  it && i<NUM_MODELS;
			  ++i, it = it->NextSiblingElement( "Model" ))
		{
			model[i] = 0;
			GLASSERT( !resource[i] || it->FirstAttribute() );
			if ( it->FirstAttribute() ) {
				GLASSERT( resource[i] );
				model[i] = engine->AllocModel( resource[i] );
				model[i]->Load( it, engine->GetSpaceTree() );
			}
		}
	}

	const XMLElement* metaEle = element->FirstChildElement( "metaDataNames" );
	GLASSERT( metaEle );
	if ( metaEle ) {
		int i=0; 
		for( const XMLElement* it = metaEle->FirstChildElement( "metaDataName" );
			 it && i<EL_MAX_METADATA;
			 ++i, it=it->NextSiblingElement( "metaDataName" ))
		{
			metaDataName[i] = IString();
			const char* name = it->Attribute( "name" );
			if ( name ) {
				metaDataName[i] = StringPool::Intern( name );
			}
		}
	}
	this->EndLoad( element );
}

	
void RenderComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "RenderComponent" );

	printer->OpenElement( "resources" );

	int n = CountArray( resource, NUM_MODELS );
	for( int i=0; i<n; ++i ) {
		printer->OpenElement( "resource" );
		if ( resource[i] ) {
			printer->PushAttribute( "name", resource[i]->header.name.c_str() );
		}
		printer->CloseElement();	// resource
	}
	printer->CloseElement(); // resources

	printer->OpenElement( "models" );
	n = CountArray( model, NUM_MODELS );
	for( int i=0; i<n; ++i ) {
		if ( model[i] ) {
			model[i]->Save( printer );
		}
		else {
			printer->OpenElement( "Model" );
			printer->CloseElement();
		}
	}
	printer->CloseElement();	// models

	printer->OpenElement( "metaDataNames" );

	n = 0;
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( !metaDataName[i].empty() ) {
			n = i+1;
		}
	}
	for( int i=0; i<n; ++i ) {
		printer->OpenElement( "metaDataName" );
		if ( !metaDataName[i].empty() ) {
			printer->PushAttribute( "name", metaDataName[i].c_str() );
		}
		printer->CloseElement();	// metaDataName
	}
	printer->CloseElement();	// metaDataNames

	this->EndSave( printer );
}


void RenderComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( resource[i] ) {
			if ( !model[i] ) {
				model[i] = engine->AllocModel( resource[i] );
			}
			model[i]->userData = parentChit;
			model[i]->Modify();
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
		metaDataName[i] = IString();
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


int RenderComponent::CalcAnimation() const
{
	int current = model[0]->GetAnimation();
	int n = ANIM_STAND;

	// Melee is tricky. If we are just standing around,
	// can stay in melee...but motion should override.
	if ( current == ANIM_MELEE ) {
		if ( !model[0]->AnimationDone() )
			return current;
	}

	MoveComponent* move = parentChit->GetMoveComponent();
	bool isMoving = move && move->IsMoving();
	InventoryComponent* inv = GET_COMPONENT( parentChit, InventoryComponent );
	bool isCarrying = inv && inv->IsCarrying();

	if ( isMoving ) {
		if ( isCarrying ) {
			n = ANIM_GUN_WALK;
		}
		else {
			n = ANIM_WALK;
		}
	}
	else if ( n == ANIM_STAND ) {
		if ( isCarrying ) {
			n = ANIM_GUN_STAND;
		}
	}
	return n;
}


int RenderComponent::CurrentAnimation() const
{
	return model[0]->GetAnimation();
}


bool RenderComponent::AnimationReady() const
{
	int type = model[0]->GetAnimation();
	if ( type == ANIM_MELEE || type == ANIM_HEAVY_IMPACT ) {
		return model[0]->AnimationDone();
	}
	return true;
}


bool RenderComponent::PlayAnimation( int type )
{
	if ( type == ANIM_HEAVY_IMPACT ) {
		// *always* overrides
		model[0]->SetAnimation( ANIM_HEAVY_IMPACT, CROSS_FADE_TIME, true );
	}
	else if ( type == ANIM_MELEE ) {
		// melee if we can
		if ( AnimationReady() ) {
			model[0]->SetAnimation( ANIM_MELEE, CROSS_FADE_TIME, true );
			return true;
		}
	}
	else {
		GLASSERT( 0 );	// other cases aren't explicity set
	}
	return false;
}



void RenderComponent::Attach( IString metaData, const char* asset )
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
			}
			break;
		}
	}
}


void RenderComponent::SetColor( IString hardpoint, const grinliz::Vector4F& colorMult )
{
	if ( !hardpoint.empty() ) {
		for( int i=0; i<EL_MAX_METADATA; ++i ) {
			if ( metaDataName[i] == hardpoint ) {
				GLASSERT( model[i+1] );
				if ( model[i+1] ) {
					model[i+1]->SetColor( colorMult );
					return;
				}
			}
		}
		GLASSERT( 0 );	// safe, but we meant to find something.
	}
	else {
		model[0]->SetColor( colorMult );
	}
}


void RenderComponent::SetProcedural( IString hardpoint, const ProcRenderInfo& info )
{
	if ( !hardpoint.empty() ) {
		for( int i=0; i<EL_MAX_METADATA; ++i ) {
			if ( metaDataName[i] == hardpoint ) {
				GLASSERT( model[i+1] );
				if ( model[i+1] ) {
					model[i+1]->SetProcedural( true, info.color, info.vOffset );
					model[i+1]->SetBoneFilter( info.filterName, info.filter );
					return;
				}
			}
		}
		GLASSERT( 0 );	// safe, but we meant to find something.
	}
	else {
		model[0]->SetProcedural( true, info.color, info.vOffset );
		model[0]->SetBoneFilter( info.filterName, info.filter );
	}
}


void RenderComponent::Detach( IString metaData )
{
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( metaDataName[i-1] == metaData ) {
			metaDataName[i-1] = IString();
			if ( model[i] )
				engine->FreeModel( model[i] );
			model[i] = 0;
			resource[i] = 0;
		}
	}
}


int RenderComponent::DoTick( U32 deltaTime, U32 since )
{
	GRINLIZ_PERFTRACK;
	int tick = VERY_LONG_TICK;

	SpatialComponent* spatial = SyncToSpatial();

	// Animate the primary model.
	if ( spatial && model[0] && model[0]->GetAnimationResource() ) {
		tick = 0;	

		// Update to the current, correct animation if we are
		// in a slice we can change
		if ( AnimationReady() ) {
			int n = this->CalcAnimation();
			model[0]->SetAnimation( n, CROSS_FADE_TIME, false );
		}

		grinliz::CArray<int, EL_MAX_METADATA> metaData;
		model[0]->DeltaAnimation( deltaTime, &metaData, 0 );

		for( int i=0; i<metaData.Size(); ++i ) {
			if ( metaData[i] == ANIM_META_IMPACT ) {
				parentChit->SendMessage( ChitMsg( ChitMsg::RENDER_IMPACT ), this );
			}
			else {
				GLASSERT( 0 );	// event not recognized
			}
		}
	}

	// Some models are particle emitters; handle those here:
	for( int i=0; i<NUM_MODELS; ++i ) {
		if( model[i] && model[i]->HasParticles() ) {
			model[i]->EmitParticles( engine->particleSystem, engine->camera.EyeDir3(), deltaTime );
			tick = 0;
		}
	}

	// Position the attachments relative to the parent.
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			GLASSERT( !metaDataName[i-1].empty() );
			Matrix4 xform;
			model[0]->CalcMetaData( metaDataName[i-1].c_str(), &xform );
			model[i]->SetTransform( xform );
		}
	}

	if ( parentChit->GetItemComponent() ) 
		parentChit->GetItemComponent()->EmitEffect( engine, deltaTime );
	if ( parentChit->GetInventoryComponent() ) 
		parentChit->GetInventoryComponent()->EmitEffect( engine, deltaTime );

	return tick;
}


float RenderComponent::RadiusOfBase()
{
	if ( resource[0] && radiusOfBase == 0 ) {
		const Rectangle3F& b = resource[0]->AABB();
		radiusOfBase = Mean( b.SizeX(), b.SizeZ() );
		radiusOfBase = Min( radiusOfBase, MAX_BASE_RADIUS );
	}
	return radiusOfBase;
}


const char* RenderComponent::GetMetaData( int i )
{
	GLASSERT( i >= 0 && i < EL_MAX_METADATA );
	if ( resource[0] ) {
		const char* name = resource[0]->header.metaData[i].name.c_str();
		if ( name && *name )
			return name;
	}
	return 0;
}


bool RenderComponent::HasMetaData( const char* name )
{
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( resource[0] ) {
			if ( resource[0]->header.metaData[i].name == name )
				return true;
		}
	}
	return false;
}


bool RenderComponent::GetMetaData( const char* name, grinliz::Matrix4* xform )
{
	if ( model[0] ) {
		SyncToSpatial();
		model[0]->CalcMetaData( name, xform );
		return true;
	}
	return false;
}


bool RenderComponent::GetMetaData( const char* name, grinliz::Vector3F* pos )
{
	Matrix4 xform;
	if ( GetMetaData( name, &xform ) ) {
		*pos = xform * V3F_ZERO;
		return true;
	}
	return false;
}


bool RenderComponent::CalcTarget( grinliz::Vector3F* pos )
{
	if ( model[0] ) {
		if ( HasMetaData( "target" )) {
			GetMetaData( "target", pos );
		}
		else {
			*pos = model[0]->AABB().Center();
		}
		return true;
	}
	return false;
}


void RenderComponent::GetModelList( grinliz::CArray<const Model*, NUM_HARDPOINTS+2> *ignore )
{
	ignore->Clear();
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( model[i] ) 
			ignore->Push( model[i] );
	}
	ignore->Push( 0 );
}


void RenderComponent::DebugStr( GLString* str )
{
	str->Format( "[Render]=%s ", resource[0]->header.name.c_str() );
}


void RenderComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DESTROYED_START ) {
		// Don't self delete.
	}
	else if ( msg.ID() == ChitMsg::CHIT_DESTROYED_TICK ) {
		float f = msg.dataF;
		Vector4F v = { f, 1, 1, 1 };
		for( int i=0; i<NUM_MODELS; ++i ) {
			if ( model[i] ) {
				model[i]->SetControl( v );
			}
		}
	}
	else if ( msg.ID() == ChitMsg::CHIT_DESTROYED_END ) {
		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F DOWN = { 0, -1, 0 };
		static const Vector3F RIGHT = { 1, 0, 0 };
		const Vector3F* eyeDir = engine->camera.EyeDir3();
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), UP, eyeDir, 0 );
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), DOWN, eyeDir, 0 );
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}
