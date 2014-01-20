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

#include "../Shiny/include/Shiny.h"
#include "../script/procedural.h"

#include "../game/lumosgame.h"

using namespace grinliz;
using namespace tinyxml2;

RenderComponent::RenderComponent( Engine* _engine, const char* asset ) 
	: engine( _engine )
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i] = 0;
	}
	radiusOfBase = 0;
	groundMark = 0;

	mainAsset = StringPool::Intern( asset );
}


RenderComponent::~RenderComponent()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		GLASSERT( model[i] == 0 );
	}
	GLASSERT( groundMark == 0 );
}


const ModelResource* RenderComponent::MainResource() const
{
	GLASSERT( model[0] );
	return model[0]->GetResource();
}


void RenderComponent::Serialize( XStream* xs )
{
	// FIXME serialize deco
	BeginSerialize( xs, "RenderComponent" );

	XarcOpen( xs, "models" );
	for( int i=0; i<NUM_MODELS; ++i ) {
		XarcOpen( xs, "models" );
		IString asset = model[i] ? model[i]->GetResource()->IName() : IString();
		XARC_SER( xs, asset );

		if ( xs->Loading() ) {
			if ( !asset.empty() ) {
				const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( asset.c_str() );
				model[i] = engine->AllocModel( res );
				model[i]->Serialize( xs, engine->GetSpaceTree() );
			}
		}
		else {
			if ( model[i] ) {
				model[i]->Serialize( xs, engine->GetSpaceTree() );
			}
		}
		XarcClose( xs );
	}
	XarcClose( xs );

	EndSerialize( xs );
}


void RenderComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );

	if ( !model[0] ) {
		model[0] = engine->AllocModel( mainAsset.c_str() );
	}

	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
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
		}
	}
	if ( groundMark ) {
		engine->FreeModel( groundMark );
		groundMark = 0;
	}
	for( int i=0; i<icons.Size(); ++i ) {
		delete icons[i].image;
	}
	icons.Clear();
}


SpatialComponent* RenderComponent::SyncToSpatial()
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( spatial ) {
		model[0]->SetPos( spatial->GetPosition() );
		model[0]->SetRotation( spatial->GetRotation() );
	}

	const GameItem* item = parentChit->GetItem();
	if ( item ) {
		if ( item->flags & GameItem::CLICK_THROUGH )
			model[0]->SetFlag( MODEL_CLICK_THROUGH );
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

	bool isCarrying = model[HARDPOINT_TRIGGER] != 0;

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


bool RenderComponent::Attach( int metaData, const char* asset )
{
	GLASSERT( metaData > 0 && metaData < EL_NUM_METADATA );
	
	// Don't re-create if not needed.
	if ( model[metaData] && StrEqual( model[metaData]->GetResource()->Name(), asset )) {
		return true;
	}

	if ( model[metaData] ) {
		engine->FreeModel( model[metaData] );
	}

	// If we are already added (model[0] exists) add the attachments.
	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( asset );
	model[metaData] = engine->AllocModel( res );
	model[metaData]->userData = parentChit;
	return true;
}


void RenderComponent::SetColor( int metaData, const grinliz::Vector4F& colorMult )
{
	GLASSERT( metaData >= 0 && metaData < EL_NUM_METADATA );
	GLASSERT( model[metaData] );
	if ( model[metaData] ) {
		model[metaData]->SetColor( colorMult );
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



void RenderComponent::SetProcedural( int metaData, const ProcRenderInfo& info )
{
	GLASSERT( model[metaData] );
	Model* m = model[metaData];
	if ( m ) {
		m->SetTextureXForm( info.te.uvXForm );
		m->SetColorMap( true, info.color );
		m->SetBoneFilter( info.filterName, info.filter );
		return;
	}
}


void RenderComponent::Detach( int metaData )
{
	GLASSERT( metaData > 0 && metaData < EL_NUM_METADATA );
	if ( model[metaData] ) {
		engine->FreeModel( model[metaData] );
	}
	model[metaData] = 0;
}


int RenderComponent::DoTick( U32 deltaTime )
{
	//GRINLIZ_PERFTRACK;
	PROFILE_FUNC();

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
	const ModelHeader& header =  model[0]->GetResource()->header;
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			Matrix4 xform;
			model[0]->CalcMetaData( i, &xform );
			model[i]->SetTransform( xform );
		}
	}

	if ( groundMark ) {
		Vector3F pos = model[0]->Pos();
		pos.y += 0.01f;
		groundMark->SetPos( pos );
	}

	ProcessIcons( (int) deltaTime );
	return tick;
}


void RenderComponent::AddDeco( const char* asset, int duration )
{
	Icon* icon = icons.PushArr(1);
	icon->image = new gamui::Image();
	gamui::RenderAtom atom = LumosGame::CalcIconAtom( asset );
	icon->image->Init( &engine->overlay, atom, false );
	icon->time = duration;
	icon->rotation = 90.0f;
}


void RenderComponent::ProcessIcons( int delta )
{
	for( int i=0; i<icons.Size(); ++i ) {
		icons[i].time -= delta;
		icons[i].rotation -= Travel( 180.0f, float(delta)/1000.0f );

		if ( icons[i].time < 0 ) {
			delete icons[i].image;
			icons.Remove( i );
			--i;
		}
	}

	if ( icons.Empty() ) {
		return;
	}

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	Vector3F pos = { 0, 0, 0 };
	if ( sc ) pos = sc->GetPosition();

	bool inView = false;
	static const float SIZE = 20.0f;

	float len2 = ( engine->camera.PosWC() - pos ).LengthSquared();
	if ( len2 < EL_FAR*EL_FAR ) {
		const Screenport& port = engine->GetScreenport();
		
		Vector2F ui = { 0, 0 };
		const Rectangle3F& aabb = model[0]->AABB();
		Vector3F topCenter = { pos.x, aabb.max.y, pos.z };

		port.WorldToUI( topCenter, &ui );
		Rectangle2F uiBounds;
		uiBounds.Set( 0, 0, port.UIWidth(), port.UIHeight() );
		uiBounds.Outset( port.UIHeight() * 0.25f );

		if ( uiBounds.Contains( ui )) {
			inView = true;
			float width = icons.Size() * SIZE;

			for( int i=0; i<icons.Size(); ++i ) {
				icons[i].image->SetCenterPos( ui.x - width*0.5f + (float)(i)*SIZE + SIZE*0.5f, ui.y-SIZE );
				icons[i].image->SetVisible( true );
				icons[i].image->SetSize( SIZE, SIZE );

				if ( icons[i].rotation >= 0 ) {
					icons[i].image->SetRotationY( icons[i].rotation );
				}
				else {
					icons[i].image->SetRotationY( 0 );
				}
			}
		}
	}

	if ( !inView ) {
		for( int i=0; i<icons.Size(); ++i ) {
			icons[i].image->SetVisible( false );
		}
	}
}


void RenderComponent::SetGroundMark( const char* asset )
{
	if ( groundMark ) {
		engine->FreeModel( groundMark );
		groundMark = 0;
	}
	if ( asset && *asset ) {
		groundMark = engine->AllocModel( "iconPlate" );
		Vector3F pos = model[0]->Pos();
		pos.y = 0.01f;
		groundMark->SetPos( pos );	
		groundMark->userData = parentChit;
		groundMark->SetFlag( MODEL_CLICK_THROUGH );

		Texture* texture = groundMark->GetResource()->atom[0].texture;
		Texture::TableEntry te;
		texture->GetTableEntry( asset, &te );
		GLASSERT( !te.name.empty() );

		groundMark->SetTextureXForm( te.uvXForm.x, te.uvXForm.y, te.uvXForm.z, te.uvXForm.w );
		groundMark->SetTextureClip( te.clip.x, te.clip.y, te.clip.z, te.clip.w );
	}
}


bool RenderComponent::CarryHardpointAvailable()
{
	if ( HasHardpoint( HARDPOINT_TRIGGER )) {
		if ( model[0] && model[0]->GetAnimationResource()->HasAnimation( ANIM_GUN_WALK )) {
			return true;
		}
	}
	return false;
}


bool RenderComponent::HasHardpoint( int hardpoint )
{
	const ModelHeader& header = model[0]->GetResource()->header;
	return header.metaData[hardpoint].InUse();
}


const ModelResource* RenderComponent::ResourceAtHardpoint( int hardpoint )
{
	GLASSERT( hardpoint >= 0 && hardpoint < EL_NUM_METADATA );
	if ( model[hardpoint] ) {
		return model[hardpoint]->GetResource();
	}
	return 0;
}



float RenderComponent::RadiusOfBase()
{
	if ( model[0] && radiusOfBase == 0 ) {
		const Rectangle3F& b = model[0]->GetResource()->AABB();
		radiusOfBase = Mean( b.SizeX(), b.SizeZ() );
		radiusOfBase = Min( radiusOfBase, MAX_BASE_RADIUS );
	}
	return radiusOfBase;
}


bool RenderComponent::HasMetaData( int id )
{
	GLASSERT( id >= 0 && id < EL_NUM_METADATA );
	return model[0]->GetResource()->header.metaData[id].InUse();
}


bool RenderComponent::GetMetaData( int id, grinliz::Matrix4* xform )
{
	if ( model[0] ) {
		SyncToSpatial();
		model[0]->CalcMetaData( id, xform );
		return true;
	}
	return false;
}


bool RenderComponent::GetMetaData( int id, grinliz::Vector3F* pos )
{
	Matrix4 xform;
	if ( GetMetaData( id, &xform ) ) {
		*pos = xform * V3F_ZERO;
		return true;
	}
	return false;
}


bool RenderComponent::CalcTarget( grinliz::Vector3F* pos )
{
	if ( model[0] ) {
		if ( HasMetaData( 0 )) {
			GetMetaData( 0, pos );
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
		IString trigger = IStringConst::trigger;
		if ( HasMetaData( HARDPOINT_TRIGGER )) { 
			if ( pos ) 
				GetMetaData( HARDPOINT_TRIGGER, pos );
			if ( xform )
				GetMetaData( HARDPOINT_TRIGGER, xform );
			return true;
		}
	}
	return false;
}


void RenderComponent::GetModelList( grinliz::CArray<const Model*, RenderComponent::NUM_MODELS+1> *ignore )
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
	str->Format( "[Render]=%s ", model[0]->GetResource()->header.name.c_str() );
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
