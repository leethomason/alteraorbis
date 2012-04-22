#include "noisetestscene.h"

#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../game/lumosgame.h"

using namespace gamui;

NoiseTestScene::NoiseTestScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
#if 0
	const ButtonLook& stdBL = lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	for ( int i=0; i<NUM_ITEMS; ++i ) {
		itemArr[i].Init( &gamui2D, stdBL );
	}
	itemArr[0].SetDeco( LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, true ), LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, false ) );
#endif

}


void NoiseTestScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, 0 );

	//LayoutCalculator layout = lumosGame->DefaultLayout();
}


void NoiseTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}
