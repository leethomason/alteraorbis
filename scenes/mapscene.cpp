#include "mapscene.h"
#include "../game/lumosgame.h"

using namespace gamui;

MapScene::MapScene( LumosGame* game, MapSceneData* data ) : Scene( game ), lumosGame(game)
{
	game->InitStd( &gamui2D, &okay, 0 );

	lumosChitBag = data->lumosChitBag;
	worldMap = data->worldMap;
	player = data->player;

	Texture* mapTexture = TextureManager::Instance()->GetTexture( "miniMap" );
	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)mapTexture, 0, 0, 1, 1 );
	mapImage.Init( &gamui2D, mapAtom, false );
}


MapScene::~MapScene()
{
}


void MapScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
	mapImage.SetSize( 400, 400 );
	mapImage.SetPos( 50, 50 );
}



void MapScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}



