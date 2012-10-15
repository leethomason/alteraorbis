#include "livepreviewscene.h"
#include "../game/lumosgame.h"

using namespace gamui;
using namespace grinliz;

LivePreviewScene::LivePreviewScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );

}


void LivePreviewScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, 0 );

	//LayoutCalculator layout = lumosGame->DefaultLayout();
}


void LivePreviewScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}
