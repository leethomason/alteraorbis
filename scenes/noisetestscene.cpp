#include "noisetestscene.h"

#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../game/lumosgame.h"
#include "../grinliz/glstringutil.h"

using namespace gamui;
using namespace grinliz;

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

	TextureManager* texman = TextureManager::Instance();
	if ( !texman->IsTexture( "noise" ) ) {
		texman->CreateTexture( "noise", SIZE, SIZE, Surface::RGB16, Texture::PARAM_NONE, this );
	}

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, texman->GetTexture( "noise" ), 0, 0, 1, 1 );
	noiseImage.Init( &gamui2D, atom, false );
	noiseImage.SetSize( 200, 200 );
}


void NoiseTestScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, 0 );
	noiseImage.SetPos( 0, 0 );

	//LayoutCalculator layout = lumosGame->DefaultLayout();
}


void NoiseTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void NoiseTestScene::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "noise" ) ) {
		
		for( int i=0; i<SIZE*SIZE; ++i ) {
			buffer[i] = random.Rand();
		}
		t->Upload( buffer, SIZE*SIZE*sizeof(buffer[0]) );
	}
	else {
		GLASSERT( 0 );
	}
}

