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

#include "lumosgame.h"
#include "gamelimits.h"
#include "worldinfo.h"
#include "../markov/markov.h"

#include "../scenes/titlescene.h"
#include "../scenes/dialogscene.h"
#include "../scenes/rendertestscene.h"
#include "../scenes/particlescene.h"
#include "../scenes/navtestscene.h"
#include "../scenes/navtest2scene.h"
#include "../scenes/battletestscene.h"
#include "../scenes/animationscene.h"
#include "../scenes/livepreviewscene.h"
#include "../scenes/worldgenscene.h"
#include "../scenes/gamescene.h"

using namespace grinliz;
using namespace gamui;

LumosGame::LumosGame(  int width, int height, int rotation, const char* savepath ) 
	: Game( width, height, rotation, 600, savepath )
{
	InitButtonLooks();
	ItemDefDB::Instance()->Load( "./res/itemdef.xml" );

	PushScene( SCENE_TITLE, 0 );
	PushPopScene();
}


LumosGame::~LumosGame()
{
	TextureManager::Instance()->TextureCreatorInvalid( this );
	delete ItemDefDB::Instance();
}


void LumosGame::InitButtonLooks()
{
	TextureManager* tm = TextureManager::Instance();

	RenderAtom blueUp(	(const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
						(const void*)tm->GetTexture( "icons" ), 
						0.125f, 0, 0.250f, 0.250f );
	RenderAtom blueUpD = blueUp;
	blueUpD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	RenderAtom blueDown((const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
						(const void*)tm->GetTexture( "icons" ), 
						0.125f, 0.250f, 0.250f, 0.500f );
	RenderAtom blueDownD = blueDown;
	blueDownD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	buttonLookArr[ BUTTON_LOOK_STD ].Init( blueUp, blueUpD, blueDown, blueDownD );
}


Scene* LumosGame::CreateScene( int id, SceneData* data )
{
	Scene* scene = 0;

	switch ( id ) {
	case SCENE_TITLE:		scene = new TitleScene( this );				break;
	case SCENE_DIALOG:		scene = new DialogScene( this );			break;
	case SCENE_RENDERTEST:	scene = new RenderTestScene( this, (const RenderTestSceneData*)data );		break;
	case SCENE_PARTICLE:	scene = new ParticleScene( this );			break;
	case SCENE_NAVTEST:		scene = new NavTestScene( this );			break;
	case SCENE_NAVTEST2:	scene = new NavTest2Scene( this, (const NavTest2SceneData*)data );			break;
	case SCENE_BATTLETEST:	scene = new BattleTestScene( this );		break;
	case SCENE_ANIMATION:	scene = new AnimationScene( this );			break;
	case SCENE_LIVEPREVIEW:	scene = new LivePreviewScene( this, (const LivePreviewSceneData*)data );	break;
	case SCENE_WORLDGEN:	scene = new WorldGenScene( this );			break;
	case SCENE_GAME:		scene = new GameScene( this );				break;

	default:
		GLASSERT( 0 );
		break;
	}
	return scene;
}


void LumosGame::CreateTexture( Texture* t )
{
	Game::CreateTexture( t );
}


RenderAtom LumosGame::CalcParticleAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 16 );
	Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
	int y = id / 4;
	int x = id - y*4;
	float tx0 = (float)x / 4.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/4.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIconAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons" );
	int y = id / 8;
	int x = id - y*8;
	float tx0 = (float)x / 8.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/8.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIcon2Atom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons2" );

	static const int CX = 4;
	static const int CY = 4;

	int y = id / CX;
	int x = id - y*CX;
	float tx0 = (float)x / (float)CX;
	float ty0 = (float)y / (float)CY;;
	float tx1 = tx0 + 1.f/(float)CX;
	float ty1 = ty0 + 1.f/(float)CY;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcDecoAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id <= 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "iconDeco" );
	float tx0 = 0;
	float ty0 = 0;
	float tx1 = 0;
	float ty1 = 0;

	if ( id != DECO_NONE ) {
		int y = id / 8;
		int x = id - y*8;
		tx0 = (float)x / 8.f;
		ty0 = (float)y / 4.f;
		tx1 = tx0 + 1.f/8.f;
		ty1 = ty0 + 1.f/4.f;
	}
	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_DECO : UIRenderer::RENDERSTATE_UI_DECO_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcPaletteAtom( int x, int y )
{
	const Game::Palette* p = Game::GetMainPalette();

	float u = ((float)x+0.5f)/(float)p->dx;
	float v = 1.0f - ((float)y+0.5f)/(float)p->dy;

	Vector2I c = { 0, 0 };
	Texture* texture = TextureManager::Instance()->GetTexture( "palette" );

	RenderAtom atom( (const void*)(UIRenderer::RENDERSTATE_UI_NORMAL), (const void*)texture, u, v, u, v );
	return atom;
}


/*
	640x480 mininum screen.
	6 buttons
	80 pixels / per
*/
gamui::LayoutCalculator LumosGame::DefaultLayout()
{
	const Screenport& port = GetScreenport();
	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetGutter( 10.0f, 10.0f );
	layout.SetSize( LAYOUT_SIZE, LAYOUT_SIZE );
	layout.SetSpacing( 5.0f );
	return layout;
}


void LumosGame::InitStd( gamui::Gamui* g, gamui::PushButton* okay, gamui::PushButton* cancel )
{
	const ButtonLook& stdBL = GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	gamui::LayoutCalculator layout = DefaultLayout();

	if ( okay ) {
		okay->Init( g, stdBL );
		okay->SetSize( LAYOUT_SIZE, LAYOUT_SIZE );
		okay->SetDeco( CalcDecoAtom( DECO_OKAY, true ), CalcDecoAtom( DECO_OKAY, false ) );
	}
	if ( cancel ) {
		cancel->Init( g, stdBL );
		cancel->SetSize( LAYOUT_SIZE, LAYOUT_SIZE );
		cancel->SetDeco( CalcDecoAtom( DECO_CANCEL, true ), CalcDecoAtom( DECO_CANCEL, false ) );
	}
}


void LumosGame::PositionStd( gamui::PushButton* okay, gamui::PushButton* cancel )
{
	gamui::LayoutCalculator layout = DefaultLayout();

	if ( okay )
		layout.PosAbs( okay, LumosGame::OKAY_X, -1 );
	
	if ( cancel )
		layout.PosAbs( cancel, LumosGame::CANCEL_X, -1 );
}


void LumosGame::CopyFile( const char* src, const char* target )
{
	FILE* fp = fopen( src, "rb" );
	GLASSERT( fp );
	if ( fp ) {
		FILE* tp = fopen( target, "wb" );
		GLASSERT( tp );

		if ( fp && tp ) {
			CDynArray<U8> buf;
			fseek( fp, 0, SEEK_END );
			int size = ftell( fp );
			buf.PushArr( size );

			fseek( fp, 0, SEEK_SET );
			fread( &buf[0], 1, size, fp );

			fwrite( buf.Mem(), 1, size, tp );

			fclose( tp );
		}
		fclose( fp );
	}
}



const char* LumosGame::GenName( const char* dataset, int seed, int min, int max )
{
	const gamedb::Item* parent = this->GetDatabase()->Root()->Child( "markovName" );
	GLASSERT( parent );
	const gamedb::Item* item = parent->Child( dataset );
	GLASSERT( item );
	if ( !item ) return 0;

	int size=0;
	const void* data = this->GetDatabase()->AccessData( item, "triplets", &size );
	MarkovGenerator gen( (const char*)data, size, seed );

	int len = max+1;
	int error = 100;
	while ( error-- ) {
		if ( gen.Name( &nameBuffer, max ) && (int)nameBuffer.size() >= min ) {
			return nameBuffer.c_str();
		}
	}
	return 0;
}

