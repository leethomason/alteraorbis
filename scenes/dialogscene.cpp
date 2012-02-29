#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../game/lumosgame.h"

using namespace gamui;

DialogScene::DialogScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	lumosGame->InitStd( &gamui2D, &okay, 0 );
}


void DialogScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );
}


void DialogScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}
