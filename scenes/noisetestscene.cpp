/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if 0
#include "noisetestscene.h"

#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"

#include "../game/lumosgame.h"

#include "../grinliz/glstringutil.h"
#include "../grinliz/glnoise.h"

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
		
		PerlinNoise noise( 0 );
		
		static float BASE0 = 1.f/32.f;
		static float BASE1 = 1.f/16.f;

		for( int j=0; j<SIZE; ++j ) {
			for( int i=0; i<SIZE; ++i ) {
				
				float n0 = noise.Noise( BASE0*(float)i, BASE0*(float)j, 0.0f );
				float n1 = noise.Noise( BASE1*(float)i, BASE1*(float)j, 1.0f );
				float n = n0 + n1*0.5f;
				n = PerlinNoise::Normalize( n );

				int gray = LRintf( n*255.f );
				Color4U8 color = { gray, gray, gray, 255 };
				U16 c = Surface::CalcRGB16( color );
				buffer[j*SIZE+i] = c;
			}
		}
		t->Upload( buffer, SIZE*SIZE*sizeof(buffer[0]) );
	}
	else {
		GLASSERT( 0 );
	}
}

#endif
