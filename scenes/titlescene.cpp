#include "titlescene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../game/lumosgame.h"

using namespace gamui;

TitleScene::TitleScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	LayoutCalculator layout = lumosGame->DefaultLayout();

	label.Init( &gamui2D );
	label.SetText( "Hello Lumos" );

	RenderAtom batom = game->CreateRenderAtom( UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, "title" );
	background.Init( &gamui2D, batom, false );

	lumosGame->InitStd( &gamui2D, &okay, &cancel );

	dialog.Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
	dialog.SetText( "dialog" );
	dialog.SetSize( layout.Width(), layout.Height() );
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	label.SetPos( 10, 10 );
	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );

	lumosGame->PositionStd( &okay, &cancel );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &dialog, 0, 1 );
}


void TitleScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &dialog ) {
		game->PushScene( LumosGame::SCENE_DIALOG, 0 );
	}
}



