#include "titlescene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"

#include "../game/lumosgame.h"

using namespace gamui;

TitleScene::TitleScene( LumosGame* game ) : Scene( game )
{
	label.Init( &gamui2D );
	label.SetText( "Hello Lumos" );

//	RenderAtom batom = game->CreateRenderAtom( "title" );

	TextureManager* tm = TextureManager::Instance();
	RenderAtom batom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					  tm->GetTexture( "title" ),
					  0, 0, 1, 1 );
	background.Init( &gamui2D, batom, false );
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	label.SetPos( 10, 10 );
	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );
}



