#include "gamescene.h"
#include "../game/lumosgame.h"
#include "../game/sim.h"

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	lumosGame = game;
	game->InitStd( &gamui2D, &okay, 0 );
	sim = new Sim( lumosGame );

	game->LoadGame();
}


GameScene::~GameScene()
{
	delete sim;
}


void GameScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
}



void GameScene::Zoom( int style, float delta )
{
/*	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
*/
}

void GameScene::Rotate( float degrees )
{
//	engine->camera.Orbit( degrees );
}


void GameScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
//	if ( !uiHasTap ) {
//		bool tap = Process3DTap( action, view, world, engine );
//	}
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void GameScene::HandleHotKey( int mask )
{
	super::HandleHotKey( mask );
}


void GameScene::DoTick( U32 delta )
{
}


void GameScene::Draw3D( U32 deltaTime )
{
}


void GameScene::DrawDebugText()
{
}

