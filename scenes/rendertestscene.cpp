#include "rendertestscene.h"
#include "../game/lumosgame.h"

RenderTestScene::RenderTestScene( LumosGame* game ) : Scene( game ), lumosGame( game )
{
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "maleMarine" );
	model = engine->AllocModel( res );
	engine->CameraLookAt( 0, 0, 10 );

	lumosGame->InitStd( &gamui2D, &okay, 0 );
}


RenderTestScene::~RenderTestScene()
{
	engine->FreeModel( model );
	delete engine;
}


void RenderTestScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
}


void RenderTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void RenderTestScene::Draw3D()
{
	engine->Draw();
}
