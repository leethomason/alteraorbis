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
#include "../engine/settings.h"
#include "../engine/engine.h"

#include "../xegame/testmap.h"
#include "../xarchive/glstreamer.h"

#include "../game/lumosgame.h"
#include "../game/layout.h"
#include "../game/team.h"

#include "../script/battlemechanics.h"
#include "../script/procedural.h"

#include "../audio/xenoaudio.h"

using namespace gamui;
using namespace grinliz;

TitleScene::TitleScene(LumosGame* game) : Scene(game), lumosGame(game), screenport(game->GetScreenport()), ticker(200)
{
	LayoutCalculator layout = lumosGame->DefaultLayout();

	RenderAtom batom = game->CreateRenderAtom( UIRenderer::RENDERSTATE_UI_NORMAL, "title" );
	background.Init( &gamui2D, batom, false );

	static const char* testSceneName[NUM_TESTS] = { "Dialog", 
													"Render0", "Render1", 
													"Particle", 
													"Nav", "Nav2", 
													"Battle", 
													"Animation", 
													"Asset\nPreview",
													"Sound" };

	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		testScene[i].SetText( testSceneName[i] );
	}

	static const char* gameSceneName[NUM_GAME] = { "Generate\nNew", /*"Load\nEstablished",*/ "Continue" };
	for( int i=0; i<NUM_GAME; ++i ) {
		gameScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		gameScene[i].SetText( gameSceneName[i] );
	}

	note.Init( &gamui2D );
	//note.SetText( "Thanks for playing this alpha-of-the-beta of Altera! The domain "
	//			  "vs. world part of the game is in place. "
	//			  "Please see the README.txt for information and a link to the game wiki." );

	audioButton.Init(&gamui2D, lumosGame->GetButtonLook(LumosGame::BUTTON_LOOK_STD));
	if (SettingsManager::Instance()->AudioOn()) {
		audioButton.SetDown();
	}
	else {
		audioButton.SetUp();
	}
	SetAudioButton();

	testMap = new TestMap(64,62);
	const Game::Palette* palette = game->GetPalette();
	Color3F c = palette->Get3F(0, 6);
	testMap->SetColor(c);

	engine = new Engine(&screenport, game->GetDatabase(), testMap);
	engine->LoadConfigFiles("./res/particles.xml", "./res/lighting.xml");

	engine->lighting.direction.Set(0.3f, 1, 1);
	engine->lighting.direction.Normalize();

	model[TROLL]		= engine->AllocModel("troll");
	model[MANTIS]		= engine->AllocModel("mantis");
	model[HUMAN_MALE]	= engine->AllocModel("humanMale");
	model[HUMAN_FEMALE] = engine->AllocModel("humanFemale");
	model[RED_MANTIS]	= engine->AllocModel("redmantis");
	model[CYCLOPS]		= engine->AllocModel("cyclops");
	//temple				= engine->AllocModel("pyramid0");

	Vector3F forward = { 0, 0, 1 };
	Vector3F across = { 0.4f, 0, 0 };

	static const float STEP = 0.4f;
	static const float ANGLE = 5.f;
	for (int i = 0; i < NUM_MODELS; ++i) {
		Vector3F v = { 30.5f, 0, 39.5f };
		v = v + float(i) * across;
		if (i < HUMAN_MALE)	{
			v = v + float(i)*forward;
			model[i]->SetYRotation(ANGLE);
		}
		if (i >= HUMAN_MALE) {
			v = v + float(HUMAN_MALE - 1)*forward - float(i - HUMAN_MALE)*forward;
			model[i]->SetYRotation(-ANGLE);
		}
		model[i]->SetPos(v);
		model[i]->SetFlag(Model::MODEL_INVISIBLE);
	}

	float x = Mean(model[HUMAN_FEMALE]->Pos().x, model[HUMAN_MALE]->Pos().x);
	const Vector3F CAM    = { x, 0.5,  47.0 };
	const Vector3F TARGET = { x, 0.7f, 42.5f };
	seed = 0;
	//temple->SetPos(x, 0, 38);

	engine->CameraLookAt(CAM, TARGET);
	{
		HumanGen gen(true, 99, TEAM_HOUSE0, false);
		ProcRenderInfo info;
		gen.AssignSuit(&info);
		model[HUMAN_FEMALE]->SetTextureXForm(info.te.uvXForm);
		model[HUMAN_FEMALE]->SetColorMap(info.color);
	}
	{
		HumanGen gen(false, 1, TEAM_HOUSE0, false);
		ProcRenderInfo info;
		gen.AssignSuit(&info);
		model[HUMAN_MALE]->SetTextureXForm(info.te.uvXForm);
		model[HUMAN_MALE]->SetColorMap(info.color);
	}
}


TitleScene::~TitleScene()
{
	DeleteEngine();
}


Color4F TitleScene::ClearColor()
{
	const Game::Palette* palette = game->GetPalette();
	return palette->Get4F(0, 5);
}


void TitleScene::DeleteEngine()
{
	for (int i = 0; i < NUM_MODELS; ++i) {
		engine->FreeModel(model[i]);
		model[i] = 0;
	}
//	engine->FreeModel(temple);
	delete engine;
	delete testMap;

	engine = 0;
	testMap = 0;
}


void TitleScene::Resize()
{
	const Screenport& port = game->GetScreenport();

	// Dowside of a local Engine: need to resize it.
	if (engine) {
		engine->GetScreenportMutable()->Resize(port.PhysicalWidth(), port.PhysicalHeight());
		for (int i = 0; i < NUM_MODELS; ++i) {
			if (model[i])
				model[i]->SetFlag(Model::MODEL_INVISIBLE);
		}
	}
	background.SetPos( 0, 0 );

	float aspect = port.UIAspectRatio();
	if ( aspect >= 0.5f ) {
		background.SetSize( port.UIWidth(), port.UIWidth()*0.5f );
	}
	else {
		background.SetSize( port.UIHeight()*2.0f, port.UIHeight() );
	}
	background.SetCenterPos( port.UIWidth()*0.5f, port.UIHeight()*0.5f );

	bool visible = game->GetDebugUI();
	LayoutCalculator layout = lumosGame->DefaultLayout();
	int c = 0;
	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].SetVisible( visible );

		int y = c / TESTS_PER_ROW;
		int x = c - y*TESTS_PER_ROW;
		layout.PosAbs( &testScene[i], x, y );
		++c;
	}
	layout.SetSize( LAYOUT_SIZE_X, LAYOUT_SIZE_Y );

	layout.PosAbs(&gameScene[GENERATE_WORLD], -1, -1);
	//layout.PosAbs(&gameScene[DEFAULT_WORLD], -1, -1);
	layout.PosAbs(&gameScene[CONTINUE], 0, -1, 2, 1);

	gameScene[CONTINUE].SetEnabled( false );
	const char* datPath = game->GamePath( "game", 0, "dat" );
	const char* mapPath = game->GamePath( "map",  0, "dat" );

	GLString fullpath;
	GetSystemPath(GAME_SAVE_DIR, mapPath, &fullpath);
	FILE* fp = fopen(fullpath.c_str(), "rb");
	if (fp) {
		StreamReader reader(fp);
		if (reader.Version() == CURRENT_FILE_VERSION) {
			gameScene[CONTINUE].SetEnabled(true);
		}
		fclose(fp);
	}

	layout.PosAbs( &note, 0, 1 );
	note.SetVisible(!visible);
	note.SetBounds( port.UIWidth() / 2, 0 );

	layout.PosAbs(&audioButton, -1, 0);
}


void TitleScene::SetAudioButton()
{
	if (SettingsManager::Instance()->AudioOn()) {
		audioButton.SetDeco(lumosGame->CalcUIIconAtom("audioOn", true), lumosGame->CalcUIIconAtom("audioOn", false));
	}
	else {
		audioButton.SetDeco(lumosGame->CalcUIIconAtom("audioOff", true), lumosGame->CalcUIIconAtom("audioOff", false));
	}
}


void TitleScene::ItemTapped( const gamui::UIItem* item )
{
	XenoAudio::Instance()->SetAudio(SettingsManager::Instance()->AudioOn());
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
	else if ( item == &testScene[TEST_ASSETPREVIEW] ) {
		game->PushScene( LumosGame::SCENE_LIVEPREVIEW, new LivePreviewSceneData( false ) );
	}
	else if (item == &testScene[TEST_SOUND]) {
		game->PushScene(LumosGame::SCENE_SOUND, 0);
	}
	else if ( item == &gameScene[GENERATE_WORLD] ) {
		game->PushScene( LumosGame::SCENE_WORLDGEN, 0 );
	}
	/*
	else if ( item == &gameScene[DEFAULT_WORLD] ) {
		const char* datPath = game->GamePath( "save/game_def", 0, "dat" );
		const char* mapPath = game->GamePath( "save/map_def", 0, "dat" );
		const char* pngPath = game->GamePath( "save/map_def", 0, "png" );

		const char* targetDatPath = game->GamePath( "game", 0, "dat" );
		const char* targetMapPath = game->GamePath( "map", 0, "dat" );
		const char* targetPNGPath = game->GamePath( "map", 0, "png" );

		lumosGame->CopyFile( datPath, targetDatPath );
		lumosGame->CopyFile( mapPath, targetMapPath );
		lumosGame->CopyFile( pngPath, targetPNGPath );

		game->PushScene( LumosGame::SCENE_GAME, 0 );
	}
	*/
	else if ( item == &gameScene[CONTINUE] ) {
		game->PushScene( LumosGame::SCENE_GAME, 0 );
	}
	else if (item == &audioButton) {
		SettingsManager::Instance()->SetAudioOn(audioButton.Down());
		SetAudioButton();
	}

	// If any scene gets pushed, throw away the engine resources.
	// Don't want to have an useless engine sitting around at
	// the top of the scene stack.
	if (game->IsScenePushed()) {
		DeleteEngine();
	}
}

void TitleScene::HandleHotKey(int key)
{
	if (key == GAME_HK_SPACE) {
		seed++;
		GLOUTPUT(("Seed=%d\n", seed));
		if (model[HUMAN_FEMALE]) {
			HumanGen gen(true, seed, TEAM_HOUSE0, false);
			ProcRenderInfo info;
			gen.AssignSuit(&info);
			model[HUMAN_FEMALE]->SetTextureXForm(info.te.uvXForm);
			model[HUMAN_FEMALE]->SetColorMap(info.color);
		}
	}
	else {
		super::HandleHotKey(key);
	}
}


void TitleScene::Draw3D(U32 deltaTime)
{
	if (!engine) return;

	// we use our own screenport
	screenport.SetPerspective();
	engine->Draw(deltaTime, 0, 0);
	screenport.SetUI();
}


void TitleScene::DoTick(U32 delta)
{
	if (ticker.Delta(delta)) {
		if (model[0]) {
			for (int i = 0; i < NUM_MODELS; ++i) {
				if (model[i]->Flags() & Model::MODEL_INVISIBLE) {
					model[i]->ClearFlag(Model::MODEL_INVISIBLE);
					break;
				}
			}
		}
	}
}
