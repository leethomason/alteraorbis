#include "rendertestscene.h"
#include "../game/lumosgame.h"

RenderTestScene::RenderTestScene( LumosGame* game ) : Scene( game ), lumosGame( game )
{
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "maleMarine" );
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i] = engine->AllocModel( res );
		model[i]->SetPos( 0, 0, (float)i );
		model[i]->SetRotation( (float)(-i*10) );
	}
	engine->CameraLookAt( 0, (float)(NUM_MODELS/2), 12 );

	lumosGame->InitStd( &gamui2D, &okay, 0 );
}


RenderTestScene::~RenderTestScene()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		engine->FreeModel( model[i] );
	}
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
