#include "animationscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../engine/model.h"

using namespace gamui;
using namespace grinliz;

AnimationScene::AnimationScene( LumosGame* game ) : Scene( game )
{
	currentBone = -1;

	game->InitStd( &gamui2D, &okay, 0 );
	Screenport* port = game->GetScreenportMutable();
	
	LayoutCalculator layout = game->DefaultLayout();

	boneLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneLeft.SetSize( layout.Width(), layout.Height() );
	boneLeft.SetText( "<" );

	boneRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneRight.SetSize( layout.Width(), layout.Height() );
	boneRight.SetText( ">" );	

	boneName.Init( &gamui2D );
	boneName.SetText( "all" );


	engine = new Engine( port, game->GetDatabase(), 0 );

	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "humanFemale" );
	GLASSERT( res );
	model = engine->AllocModel( res );
	model->SetPos( 1, 0, 1 );
	engine->CameraLookAt( 1, 1, 5 );
}


AnimationScene::~AnimationScene()
{
	engine->FreeModel( model );
	delete engine;
}


void AnimationScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &boneLeft, 0, -2 );
	layout.PosAbs( &boneName, 1, -2 );
	layout.PosAbs( &boneRight, 2, -2 );
}


void AnimationScene::UpdateBoneInfo()
{
	if ( currentBone == -1 ) {
		boneName.SetText( "all" );
		model->ClearParam();
	}
	else {
		GLString str;
		str.Format( "%d", currentBone );
		boneName.SetText( str.c_str() );
		model->SetBoneFilter( currentBone );
	}
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

	UpdateBoneInfo();
}


void AnimationScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
