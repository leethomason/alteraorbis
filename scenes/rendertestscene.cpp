#include "rendertestscene.h"
#include "../game/lumosgame.h"

RenderTestScene::RenderTestScene( LumosGame* game ) : Scene( game ), lumosGame( game )
{
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	testMap = new TestMap( 4, 4 );
	engine->SetMap( testMap );
	
	const ModelResource* res0 = ModelResourceManager::Instance()->GetModelResource( "femaleMarine" );
	const ModelResource* res1 = ModelResourceManager::Instance()->GetModelResource( "maleMarine" );

	model[0] = engine->AllocModel( res0 );
	model[0]->SetPos( 0, 0, 0 );

	for( int i=1; i<NUM_MODELS; ++i ) {
		model[i] = engine->AllocModel( res1 );
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
	delete testMap;
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
