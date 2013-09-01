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
#include "chit.h"
#include "chitevent.h"
#include "itemcomponent.h"
#include "istringconst.h"

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
	: engine( _engine ),
	  firstInit( false )
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
	for( int i=0; i<NUM_DECO; ++i ) {
		deco[i] = 0;
		decoDuration[i] = 0;
	}
	radiusOfBase = 0;
}


RenderComponent::~RenderComponent()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		GLASSERT( model[i] == 0 );
	}
	for( int i=0; i<NUM_DECO; ++i ) {
		GLASSERT( deco[i] == 0 );
	}
}


void RenderComponent::Serialize( XStream* xs )
{
	// FIXME serialize deco
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
	for( int i=0; i<NUM_DECO; ++i ) {
		if ( deco[i] ) {
			engine->FreeModel( deco[i] );
			deco[i] = 0;
		}
	}

}


void RenderComponent::FirstInit()
{
	GameItem* item = parentChit->GetItem();
	if ( item ) {
		if (item->flags & GameItem::CLICK_THROUGH) {
			for( int i=0; i<NUM_MODELS; ++i ) {
				if ( model[i] ) {
					model[i]->SetFlag( MODEL_CLICK_THROUGH );
				}
			}
		}
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
	ItemComponent* inv = parentChit->GetItemComponent();
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
	if ( type == ANIM_MELEE || type == ANIM_IMPACT ) {
		return model[0]->AnimationDone();
	}
	return true;
}


bool RenderComponent::PlayAnimation( int type )
{
	if ( type == ANIM_IMPACT ) {
		// *always* overrides
		model[0]->SetAnimation( ANIM_IMPACT, CROSS_FADE_TIME, true );
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


bool RenderComponent::Attach( int hardpoint, const char* asset )
{
	IString n = IStringConst::Hardpoint( hardpoint );
	return Attach( n, asset );
}

bool RenderComponent::Attach( IString metaData, const char* asset )
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
			return true;
		}
	}
	return false;
}


void RenderComponent::SetColor( int hardpoint, const grinliz::Vector4F& colorMult )
{
	IString n = IStringConst::Hardpoint( hardpoint );
	SetColor( n, colorMult );
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


void RenderComponent::SetSaturation( float s )
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			model[i]->SetSaturation( s );
		}
	}
}



void RenderComponent::SetProcedural( int hardpoint, const ProcRenderInfo& info )
{
	IString n = IStringConst::Hardpoint( hardpoint );
	SetProcedural( n, info );
}

void RenderComponent::SetProcedural( IString hardpoint, const ProcRenderInfo& info )
{
	if ( !hardpoint.empty() ) {
		for( int i=0; i<EL_MAX_METADATA+1; ++i ) {
			// Model[0] is the main model, model[1] is the first hardpoint
			if (    ( i==0 && hardpoint == IStringConst::kmain )
				 || ( i>0  && metaDataName[i-1] == hardpoint ))
			{
				GLASSERT( model[i] );
				Model* m = model[i];
				if ( m ) {
					m->SetTextureXForm( info.te.uvXForm );
					m->SetColorMap( true, info.color );
					m->SetBoneFilter( info.filterName, info.filter );
					return;
				}
			}
		}
		GLASSERT( 0 );	// safe, but we meant to find something.
	}
	else {
		model[0]->SetTextureXForm();
		model[0]->SetColorMap( false, info.color );
		model[0]->SetBoneFilter( info.filterName, info.filter );
	}
}


void RenderComponent::Detach( int metaData )
{
	IString n = IStringConst::Hardpoint( metaData );
	Detach( n );
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

	if ( !firstInit ) {
		FirstInit();
		firstInit = true;
	}

	// Animate the primary model.
	if ( spatial && model[0] && model[0]->GetAnimationResource() ) {
		tick = 0;	

		// Update to the current, correct animation if we are
		// in a slice we can change
		if ( AnimationReady() ) {
			int n = this->CalcAnimation();
			model[0]->SetAnimation( n, CROSS_FADE_TIME, false );
		}

		int metaData=0;
		model[0]->DeltaAnimation( deltaTime, &metaData, 0 );

		if ( metaData ) {
			if ( metaData == ANIM_META_IMPACT ) {
				//GLOUTPUT(( "Sending impact.\n" ));
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
			model[0]->CalcMetaData( metaDataName[i-1], &xform );
			model[i]->SetTransform( xform );
		}
	}

	// The decos:
	for( int i=0; i<NUM_DECO; ++i ) {
		if ( deco[i] ) {
			decoDuration[i] -= since;
			if ( decoDuration[i] < 0 ) {
				engine->FreeModel( deco[i] );
				deco[i] = 0;
				decoDuration[i] = 0;
			}
		}
	}

	if ( deco[DECO_FOOT] ) {
		Vector3F pos = model[0]->Pos();
		pos.y += 0.01f;
		deco[DECO_FOOT]->SetPos( pos );
		tick = 0;
	}
	if ( deco[DECO_HEAD] ) {
		Vector3F pos = model[0]->Pos();
		pos.y += model[0]->AABB().SizeY() + 0.4f;

		Quaternion camera = engine->camera.Quat();
		Matrix4 camMat;
		camera.ToMatrix( &camMat );
		float degrees = camMat.CalcRotationAroundAxis( 1 );
	
		deco[DECO_HEAD]->SetPos( pos );
		deco[DECO_HEAD]->SetYRotation( degrees );
		tick = 0;
	}
	return tick;
}


void RenderComponent::Deco( const char* asset, int slot, int duration )
{
	GLASSERT( slot < NUM_DECO );
	if ( deco[slot] ) {
		engine->FreeModel( deco[slot] );
		deco[slot] = 0;
	}
	if ( duration > 0 ) {
		decoDuration[slot] = duration;
		const ModelResource* res = ModelResourceManager::Instance()
			->GetModelResource( slot == DECO_FOOT ? "iconPlate" : "iconHeadPlate" );
		deco[slot] = engine->AllocModel( res );
		Vector3F pos = model[0]->Pos();
		pos.y = 0.01f;
		deco[slot]->SetPos( pos );	
		deco[slot]->userData = parentChit;
		deco[slot]->SetFlag( MODEL_CLICK_THROUGH );

		Texture* texture = deco[slot]->GetResource()->atom[0].texture;
		Texture::TableEntry te;
		texture->GetTableEntry( asset, &te );
		GLASSERT( !te.name.empty() );

		deco[slot]->SetTextureXForm( te.uvXForm.x, te.uvXForm.y, te.uvXForm.z, te.uvXForm.w );
		deco[slot]->SetTextureClip( te.clip.x, te.clip.y, te.clip.z, te.clip.w );
	}
}


bool RenderComponent::CarryHardpointAvailable()
{
	if ( HardpointAvailable( IStringConst::ktrigger )) {
		if ( model[0] && model[0]->GetAnimationResource()->HasAnimation( ANIM_GUN_WALK )) {
			return true;
		}
	}
	return false;
}


bool RenderComponent::HardpointAvailable( int hardpoint )
{
	int has = 0;
	int use = 0;
	IString n = IStringConst::Hardpoint( hardpoint );
	return HardpointAvailable( n );
}


bool RenderComponent::HardpointAvailable( const IString& n )
{
	const ModelHeader& header = model[0]->GetResource()->header;
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( header.metaData[i].name == n ) {
			// have the slot - is it in use?
			for( int k=0;  k<EL_MAX_METADATA; ++k ) {
				if ( metaDataName[k] == n ) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
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


bool RenderComponent::HasMetaData( grinliz::IString name )
{
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( resource[0] ) {
			if ( resource[0]->header.metaData[i].name == name )
				return true;
		}
	}
	return false;
}


bool RenderComponent::GetMetaData( grinliz::IString name, grinliz::Matrix4* xform )
{
	if ( model[0] ) {
		SyncToSpatial();
		model[0]->CalcMetaData( name, xform );
		return true;
	}
	return false;
}


bool RenderComponent::GetMetaData( grinliz::IString name, grinliz::Vector3F* pos )
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
		IString target = IStringConst::ktarget;
		if ( HasMetaData( target )) {
			GetMetaData( target, pos );
		}
		else {
			*pos = model[0]->AABB().Center();
		}
		return true;
	}
	return false;
}


bool RenderComponent::CalcTrigger( grinliz::Vector3F* pos, grinliz::Matrix4* xform )
{
	if ( model[0] ) {
		IString trigger = IStringConst::ktrigger;
		if ( HasMetaData( trigger )) { 
			if ( pos ) 
				GetMetaData( trigger, pos );
			if ( xform )
				GetMetaData( trigger, xform );
			return true;
		}
	}
	return false;
}


void RenderComponent::GetModelList( grinliz::CArray<const Model*, NUM_MODELS+1> *ignore )
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

		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F DOWN = { 0, -1, 0 };
		static const Vector3F RIGHT = { 1, 0, 0 };
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), UP, 0 );
		engine->particleSystem->EmitPD( "derez", model[0]->AABB().Center(), DOWN, 0 );
	}
	else if ( msg.ID() == ChitMsg::CHIT_DESTROYED_TICK ) {
		float f = msg.dataF;
		for( int i=0; i<NUM_MODELS; ++i ) {
			if ( model[i] ) {
				model[i]->SetFadeFX( f );
			}
		}
	}
	else if ( msg.ID() == ChitMsg::CHIT_DESTROYED_END ) {
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}
