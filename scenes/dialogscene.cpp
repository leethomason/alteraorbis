#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../game/lumosgame.h"

using namespace gamui;

DialogScene::DialogScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	lumosGame->InitStd( &gamui2D, &okay, 0 );

	const ButtonLook& stdBL = lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	for ( int i=0; i<NUM_ITEMS; ++i ) {
		itemArr[i].Init( &gamui2D, stdBL );
	}
	itemArr[0].SetDeco( LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, true ), LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, false ) );
}


void DialogScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();

	for ( int i=0; i<NUM_ITEMS; ++i ) {
		layout.PosAbs( &itemArr[i], i, 0 );
	}
}


void DialogScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


gamui::RenderAtom DialogScene::DragStart( const gamui::UIItem* item )
{
	PushButton* button = (PushButton*)item;
	RenderAtom atom, nullAtom;
	button->GetDeco( &atom, 0 );
	if ( !atom.Equal( nullAtom ) ) {
		button->SetDeco( nullAtom, nullAtom );
	}
	return atom;
}


void DialogScene::DragEnd( const gamui::UIItem* start, const gamui::UIItem* end )
{
	if ( end ) {
		((PushButton*)end)->SetDeco( LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, true ), LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, false ) );
	}
	else if ( start ) {
		((PushButton*)start)->SetDeco( LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, true ), LumosGame::CalcDecoAtom( LumosGame::DECO_OKAY, false ) );
	}
}
