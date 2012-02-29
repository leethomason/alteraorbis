#include "titlescene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"

#include "../game/lumosgame.h"

using namespace gamui;

TitleScene::TitleScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	label.Init( &gamui2D );
	label.SetText( "Hello Lumos" );

	RenderAtom batom = game->CreateRenderAtom( UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, "title" );
	background.Init( &gamui2D, batom, false );

	lumosGame->InitStd( &gamui2D, &okay, &cancel );
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	label.SetPos( 10, 10 );
	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );

	lumosGame->PositionStd( &okay, &cancel );
}


void TitleScene::ItemTapped( const gamui::UIItem* item )
{

}



