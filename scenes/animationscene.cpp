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

#include "animationscene.h"

#include "../game/lumosgame.h"
#include "../game/lumosmath.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/animation.h"
#include "../engine/particle.h"

#include "../xegame/istringconst.h"

#include "../script/procedural.h"

#include <direct.h>

extern void ScreenCapture( const char* baseFilename, bool appendCount, bool trim, bool makeTransparent, grinliz::Rectangle2I* size );

using namespace gamui;
using namespace grinliz;
using namespace tinyxml2;

AnimationScene::AnimationScene( LumosGame* game ) : Scene( game )
{
	currentBone = -1;
	currentAnim = 0;

	game->InitStd( &gamui2D, &okay, 0 );
	Screenport* port = game->GetScreenportMutable();
	
	LayoutCalculator layout = game->DefaultLayout();

	boneLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneLeft.SetText( "<" );

	boneRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneRight.SetText( ">" );	

	boneName.Init( &gamui2D );
	boneName.SetText( "all" );

	static const char* ANIM_NAME[ANIM_COUNT] = { "Stand", "Walk", "GunStand", "GunWalk", "Melee", "Impact" };
	for( int i=0; i<ANIM_COUNT; ++i ) {
		animSelect[i].Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
		animSelect[i].SetText( ANIM_NAME[i] );
		animSelect[0].AddToToggleGroup( &animSelect[i] );
	}

	modelLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	modelLeft.SetText( "<-" );

	modelRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	modelRight.SetText( "->" );	

	modelName.Init( &gamui2D );
	modelName.SetText( "no model" );

	ortho.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	ortho.SetText( "ortho" );

	zeroFrame.Init( &gamui2D, game->GetButtonLook(0) );
	zeroFrame.SetText( "zero" );

	instance.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	instance.SetText( "3x" );

	speedDown.Init(&gamui2D, game->GetButtonLook(0));
	speedDown.SetText("<");
	speedUp.Init(&gamui2D, game->GetButtonLook(0));
	speedUp.SetText(">");
	speedLabel.Init(&gamui2D);
	speedLabel.SetText("Def");

	static const char* triggerLabels[NUM_TRIGGERS] = {
		"particle", "gun", "ring"
	};

	for( int i=0; i<NUM_TRIGGERS; ++i ) {
		triggerToggle[i].Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
		triggerToggle[i].SetText( triggerLabels[i] );
		if ( i > 0 ) {
			triggerToggle[0].AddToToggleGroup( &triggerToggle[i] );
		}
	}

	engine = new Engine( port, game->GetDatabase(), 0 );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	int count=0;
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i] = 0;
	}

	currentModel = 0;
	const ModelResource* const* resArr = ModelResourceManager::Instance()->GetAllResources( &count );
	for( int i=0; i<count; ++i ) {
		if ( !resArr[i]->header.animation.empty() ) {
			resourceArr.Push( resArr[i] );
			if ( resArr[i]->header.name == "humanFemale" ) {
				currentModel = resourceArr.Size()-1;
			}
		}
	}

	const ModelResource* plateRes = ModelResourceManager::Instance()->GetModelResource("timingPlate");
	for (int i = 0; i < NUM_PLATES; ++i) {
		plate[i] = engine->AllocModel(plateRes);
		plate[i]->SetPos(1, 0, float(i));
//		plate[i]->SetFlag(Model::MODEL_INVISIBLE);
	}

	triggerModel = 0;
	LoadModel();

	engine->CameraLookAt( 1, 1, 5 );
	UpdateAnimationInfo();
	UpdateModelInfo();
}


AnimationScene::~AnimationScene()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		engine->FreeModel( model[i] );
	}
	for (int i = 0; i < NUM_PLATES; ++i) {
		engine->FreeModel(plate[i]);
	}
	if ( triggerModel ) {
		engine->FreeModel( triggerModel );
	}
	delete engine;
}


void AnimationScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &boneLeft,		0, -2 );
	layout.PosAbs( &boneName,		1, -2 );
	layout.PosAbs( &boneRight,		3, -2 );
	layout.PosAbs( &ortho,			4, -2 );
	layout.PosAbs( &zeroFrame,		5, -2 );
	layout.PosAbs( &instance,		6, -2 );
	for( int i=0; i<NUM_TRIGGERS; ++i ) {
		layout.PosAbs( &triggerToggle[i], 7+i, -2 );
	}

	for( int i=0; i<ANIM_COUNT; ++i ) {
		layout.PosAbs( &animSelect[i], i, 0 );
	}

	layout.PosAbs( &modelLeft,	0, -3 );
	layout.PosAbs( &modelName,	1, -3 );
	layout.PosAbs( &modelRight,	3, -3 );

	layout.PosAbs(&speedDown, 4, -1);
	layout.PosAbs(&speedLabel, 5, -1);
	layout.PosAbs(&speedUp, 6, -1);
}


void AnimationScene::LoadModel()
{
	if ( currentModel >= resourceArr.Size() ) currentModel = 0;
	if ( currentModel < 0 ) currentModel = resourceArr.Size()-1;

	const ModelResource* res = resourceArr[currentModel];
	GLASSERT( res );
	bool colorMap = res->atom[0].texture->ColorMap();

	for( int j=0; j<NUM_MODELS; ++j ) {
		if ( model[j] ) {
			engine->FreeModel( model[j] );
		}
		model[j] = engine->AllocModel( res );
		model[j]->SetPos( 1.0f + (float)j*0.6f, 0, 1 );

		if ( colorMap ) {
			HumanGen gen( false, j+4, 1, false );
			ProcRenderInfo info;
			gen.AssignSuit( &info );
			model[j]->SetColorMap( info.color );
		}
	}
	SetModelVis();
	model[1]->SetAnimationRate( 0.8f );
	model[2]->SetAnimationRate( 1.2f );

	CStr<16> str;
	str.Format("%.2f", model[0]->GetResource()->header.animationSpeed);
	speedLabel.SetText(str.c_str());
}


void AnimationScene::UpdateBoneInfo()
{
	if ( currentBone == -1 ) {
		boneName.SetText( "all" );
		for( int i=0; i<NUM_MODELS; ++i )
			model[i]->SetBoneFilter( 0 );
	}
	else {
		GLString str;
		IString name = model[0]->GetBoneName( currentBone );
		str.Format( "%d:%s", currentBone, !name.empty() ? name.c_str() : "none" );
		boneName.SetText( str.c_str() );
	
		int id[4] = { currentBone, -1, -1, -1 };
		for( int i=0; i<NUM_MODELS; ++i )
			model[i]->SetBoneFilter( id );
	}
}


void AnimationScene::UpdateModelInfo()
{
	const ModelResource* res = resourceArr[currentModel];
	modelName.SetText( res->header.name.c_str() );
	if ( model[0]->GetResource() != res ) {
		LoadModel();

		const AnimationResource* animRes = model[0]->GetAnimationResource();
		for( int i=0; i<ANIM_COUNT; ++i ) {
			animSelect[i].SetEnabled( animRes->HasAnimation( i ));
		}
	}
	modelName.SetText( model[0]->GetResource()->header.name.c_str() );
}


void AnimationScene::UpdateAnimationInfo()
{
	for( int k=0; k<ANIM_COUNT; ++k ) {
		if ( animSelect[k].Down() ) {
			int id = k;
			if ( !model[0]->GetAnimationResource()->HasAnimation( id )) {
				id = 0;
			}
			for( int i=0; i<NUM_MODELS; ++i ) {
				model[i]->SetAnimation( id, 500, false );
			}
			break;
		}
	}
}


void AnimationScene::SetModelVis()
{
	model[0]->ClearFlag( Model::MODEL_INVISIBLE );
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( !instance.Down() ) {
			model[i]->SetFlag( Model::MODEL_INVISIBLE );
		}
		else {
			model[i]->ClearFlag( Model::MODEL_INVISIBLE );
		}
	}
}


void AnimationScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void AnimationScene::Rotate( float degrees )
{
	engine->camera.Orbit( degrees );
}


void AnimationScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &boneRight ) {
		++currentBone;
		if ( currentBone == EL_MAX_BONES ) currentBone = -1;
	}
	else if ( item == &boneLeft ) {
		--currentBone;
		if ( currentBone == -2 ) currentBone = EL_MAX_BONES-1;
	}
	else if ( item == &ortho ) {
		Screenport* port = engine->GetScreenportMutable();
		if ( ortho.Down() ) {
			port->SetOrthoCamera( true, 0, 3.0f );
			static const Vector3F DIR = { 1, 0, 0 };
			static const Vector3F UP  = { 0, 1, 0 };
			engine->camera.SetPosWC( -5, 0.55f, 1 );
			engine->camera.SetDir( DIR, UP ); 
			SetModelVis();
		}
		else {
			port->SetOrthoCamera( false, 0, 0 );
			engine->CameraLookAt( 1, 1, 5 );
			SetModelVis();
		}
	}
	else if ( item == &instance ) {
		SetModelVis();
	}
	else if ( item == &modelRight ) {
		++currentModel;
		if ( currentModel == resourceArr.Size() ) currentModel = 0;
		currentAnim = 0;
	}
	else if ( item == &modelLeft ) {
		--currentModel;
		if ( currentModel < 0 ) currentModel = resourceArr.Size()-1;
		currentAnim = 0;
	}
	else if (item == &speedDown || item == &speedUp) {
		float bias = item == &speedDown ? -0.1f : 0.1f;
		float rate = model[0]->GetAnimationRate() + bias;
		model[0]->SetAnimationRate(rate);
		CStr<16> str;
		str.Format("%.2f", rate);
		speedLabel.SetText(str.c_str());
	}

	UpdateModelInfo();
	UpdateAnimationInfo();
	UpdateBoneInfo();
}


void AnimationScene::DoTick( U32 deltaTime )
{
	int metaDataEvent=0;

	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( zeroFrame.Down() ) {
			model[i]->SetAnimationTime( 0 );
			model[i]->DeltaAnimation( 0, 0, 0 );
		}
		else {
			model[i]->DeltaAnimation( deltaTime, (i==0) ? &metaDataEvent : 0, 0 );
			model[i]->EmitParticles( engine->particleSystem, engine->camera.EyeDir3(), deltaTime );
		}
	}

	float plateZ = plate[0]->Pos().z;
	plateZ -= Travel(DEFAULT_MOVE_SPEED, deltaTime);
	if (plateZ < -3) plateZ += 1.0f;

	for (int i = 0; i < NUM_PLATES; ++i) {
		Vector3F v = plate[i]->Pos();
		v.z = plateZ + float(i);
		plate[i]->SetPos(v);
		if (model[0]->GetAnimation() == ANIM_WALK || model[0]->GetAnimation() == ANIM_GUN_WALK)
			plate[i]->ClearFlag(Model::MODEL_INVISIBLE);
		else
			plate[i]->SetFlag(Model::MODEL_INVISIBLE);
	}

	if ( model[0]->HasAnimation() && model[0]->GetResource()->GetMetaData( HARDPOINT_TRIGGER )) {
		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F POS = { 0,0,0 };
		Matrix4 xform;
		model[0]->CalcMetaData( HARDPOINT_TRIGGER, &xform );
		Vector3F p = xform * POS;

		if ( metaDataEvent ) {
			engine->particleSystem->EmitPD( "derez", p, UP, 0 );	
		}

		if ( triggerToggle[PARTICLE].Down() ) {
			engine->particleSystem->EmitPD( "spell", p, UP, 0 ); 
			if ( triggerModel ) { engine->FreeModel( triggerModel ); triggerModel = 0;	}
		}
		else {
			const ModelResource* res = 0;
			if		( triggerToggle[GUN].Down() )			{ res = ModelResourceManager::Instance()->GetModelResource( "blaster" ); }
			else if ( triggerToggle[RING].Down() )			{ res = ModelResourceManager::Instance()->GetModelResource( "ring" ); }

			if ( triggerModel && triggerModel->GetResource() != res ) {
				engine->FreeModel( triggerModel );
				triggerModel = 0;
			}

			if( res && !triggerModel ) {
				triggerModel = engine->AllocModel( res );
			}

			if ( triggerModel ) {
				Matrix4 xform;
				model[0]->CalcMetaData( HARDPOINT_TRIGGER, &xform );
				triggerModel->SetTransform( xform );
			}
		}
	}
	else {
		if ( triggerModel ) { engine->FreeModel( triggerModel ); triggerModel = 0;	}
	}
}


void AnimationScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
