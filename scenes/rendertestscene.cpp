#include "rendertestscene.h"
#include "../game/lumosgame.h"

RenderTestScene::RenderTestScene( LumosGame* game, const RenderTestSceneData* data ) : Scene( game ), lumosGame( game )
{
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );

	for( int i=0; i<NUM_MODELS; ++i )
		model[i] = 0;

	testMap = new TestMap( 8, 8 );
	engine->SetMap( testMap );
	
	switch( data->id ) {
	case 0:
		SetupTest0();
		break;
	case 1:
		SetupTest1();
		break;
	default:
		GLASSERT( 0 );
		break;
	};
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



void RenderTestScene::SetupTest0()
{
	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "maleMarine" );
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i] = engine->AllocModel( res );
		model[i]->SetPos( 1, 0, (float)i );
		model[i]->SetRotation( (float)(-i*10) );
	}
	engine->CameraLookAt( 0, (float)(NUM_MODELS/2), 12 );
}


void RenderTestScene::SetupTest1()
{
	const ModelResource* testStruct0Res = ModelResourceManager::Instance()->GetModelResource( "testStruct0" );
	const ModelResource* testStruct1Res = ModelResourceManager::Instance()->GetModelResource( "testStruct1" );

	model[0] = engine->AllocModel( testStruct0Res );
	model[0]->SetPos( 0.5f, 0.0f, 1.0f );

	model[1] = engine->AllocModel( testStruct1Res );
	model[1]->SetPos( 0.5f, 0.0f, 3.5f );

	engine->CameraLookAt( 0, 3, 8, -45.f, -30.f );
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
