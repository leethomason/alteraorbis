#include "animationscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../engine/model.h"

AnimationScene::AnimationScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	Screenport* port = game->GetScreenportMutable();
	engine = new Engine( port, game->GetDatabase(), 0 );

	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "humanFemale" );
	GLASSERT( res );
	model = engine->AllocModel( res );
	model->SetPos( 1, 0, 1 );
	engine->CameraLookAt( 1, 1, 20 );
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
}


void AnimationScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void AnimationScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
