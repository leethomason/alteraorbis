#include "navtestscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"

NavTestScene::NavTestScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	map = new WorldMap( 20, 20 );
	map->InitCircle();

	engine->SetMap( map );
	engine->CameraLookAt( 10, 10, 40 );
}


NavTestScene::~NavTestScene()
{
	engine->SetMap( 0 );
	delete map;
	delete engine;
}


void NavTestScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );
}


void NavTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void NavTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
