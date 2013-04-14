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

#include "titlescene.h"
#include "rendertestscene.h"
#include "navtest2scene.h"
#include "livepreviewscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"

#include "../game/lumosgame.h"

#include "../script/battlemechanics.h"

using namespace gamui;
using namespace grinliz;

TitleScene::TitleScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	LayoutCalculator layout = lumosGame->DefaultLayout();

	RenderAtom batom = game->CreateRenderAtom( UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, "title" );
	background.Init( &gamui2D, batom, false );

	static const char* testSceneName[NUM_TESTS] = { "dialog", 
													"render0", "render1", 
													"particle", 
													"nav", "nav2", 
													//"Blue\nSmoke", 
													//"noise", 
													"battle", 
													"animation", 
													"Live\nPreview", 
													"Asset\nPreview" };

	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		testScene[i].SetText( testSceneName[i] );
		testScene[i].SetSize( layout.Width(), layout.Height() );
	}

	static const char* gameSceneName[NUM_GAME] = { "Generate", "Continue" };
	for( int i=0; i<NUM_GAME; ++i ) {
		gameScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		gameScene[i].SetText( gameSceneName[i] );
		gameScene[i].SetSize( layout.Width()*2.0f, layout.Height() );
	}
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );

	static const bool VIS[NUM_TESTS] = {
		false, true, false, true, false, false,
		true, true, true, true
	};
	bool visible = game->DebugUIEnabled();

	LayoutCalculator layout = lumosGame->DefaultLayout();
	int c = 0;
	for( int i=0; i<NUM_TESTS; ++i ) {
		bool v = visible || VIS[i];
		testScene[i].SetVisible( v );
		if ( v ) {
			int y = c / TESTS_PER_ROW;
			int x = c - y*TESTS_PER_ROW;
			layout.PosAbs( &testScene[i], x, y );
			++c;
		}
	}
	for( int i=0; i<NUM_GAME; ++i ) {
		layout.PosAbs( &gameScene[i], i*2, -1 );
	}
}


void TitleScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &testScene[TEST_DIALOG] ) {
		game->PushScene( LumosGame::SCENE_DIALOG, 0 );
	}
	else if ( item == &testScene[TEST_RENDER_0] ) {
		game->PushScene( LumosGame::SCENE_RENDERTEST, new RenderTestSceneData( 0 ) );
	}
	else if ( item == &testScene[TEST_RENDER_1] ) {
		game->PushScene( LumosGame::SCENE_RENDERTEST, new RenderTestSceneData( 1 ) );
	}
	else if ( item == &testScene[TEST_PARTICLE] ) {
		game->PushScene( LumosGame::SCENE_PARTICLE, 0 );
	}
	else if ( item == &testScene[TEST_NAV] ) {
		game->PushScene( LumosGame::SCENE_NAVTEST, 0 );
	}
	else if ( item == &testScene[TEST_NAV2] ) {
		game->PushScene( LumosGame::SCENE_NAVTEST2, new NavTest2SceneData( "./res/testnav.png", 100 ) );
	}
	else if ( item == &testScene[TEST_BATTLE] ) {
		game->PushScene( LumosGame::SCENE_BATTLETEST, 0 );
	}
	else if ( item == &testScene[TEST_ANIMATION] ) {
		game->PushScene( LumosGame::SCENE_ANIMATION, 0 );
	}
	else if ( item == &testScene[TEST_LIVEPREVIEW] ) {
		game->PushScene( LumosGame::SCENE_LIVEPREVIEW, new LivePreviewSceneData( true ) );
	}
	else if ( item == &testScene[TEST_ASSETPREVIEW] ) {
		game->PushScene( LumosGame::SCENE_LIVEPREVIEW, new LivePreviewSceneData( false ) );
	}
	else if ( item == &gameScene[GENERATE_WORLD] ) {
		game->PushScene( LumosGame::SCENE_WORLDGEN, 0 );
	}
	else if ( item == &gameScene[CONTINUE] ) {
		game->PushScene( LumosGame::SCENE_GAME, 0 );
	}
}



