#include "titlescene.h"
#include "rendertestscene.h"
#include "navtest2scene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"

#include "../game/lumosgame.h"

using namespace gamui;

TitleScene::TitleScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	LayoutCalculator layout = lumosGame->DefaultLayout();

	RenderAtom batom = game->CreateRenderAtom( UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, "title" );
	background.Init( &gamui2D, batom, false );

	static const char* testSceneName[NUM_TESTS] = { "dialog", 
													"render0", "render1", 
													"particle", 
													"nav", "nav2", "navWorld", 
													"noise", "battle", "animation" };

	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		testScene[i].SetText( testSceneName[i] );
		testScene[i].SetSize( layout.Width(), layout.Height() );
	}
	testScene[TEST_NAV_WORLD].SetText(  "Blue" );
	testScene[TEST_NAV_WORLD].SetText2( "Smoke" );
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	background.SetPos( 0, 0 );
	background.SetSize( port.UIWidth(), port.UIHeight() );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	for( int i=0; i<NUM_TESTS; ++i ) {
		int y = i / TESTS_PER_ROW;
		int x = i - y*TESTS_PER_ROW;
		layout.PosAbs( &testScene[i], x, y );
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
	else if ( item == &testScene[TEST_NAV_WORLD] ) {
		game->PushScene( LumosGame::SCENE_NAVTEST2, new NavTest2SceneData( "./res/testnav1024.png", 2000, 40 ) );
	}
	else if ( item == &testScene[TEST_NOISE] ) {
		game->PushScene( LumosGame::SCENE_NOISETEST, 0 );
	}
	else if ( item == &testScene[TEST_BATTLE] ) {
		game->PushScene( LumosGame::SCENE_BATTLETEST, 0 );
	}
	else if ( item == &testScene[TEST_ANIMATION] ) {
		game->PushScene( LumosGame::SCENE_ANIMATION, 0 );
	}
}



