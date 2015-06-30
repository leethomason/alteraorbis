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

#include "../version.h"

using namespace gamui;
using namespace grinliz;

TitleScene::TitleScene(LumosGame* game) : Scene(game), lumosGame(game), screenport(game->GetScreenport()), ticker(200)
{
	LayoutCalculator layout = DefaultLayout();

	RenderAtom batom;
	batom.Init((const void*)UIRenderer::RENDERSTATE_UI_NORMAL, (const void*)TextureManager::Instance()->GetTexture("title"), 0, 0, 1, 1);
	background.Init( &gamui2D, batom, false );

	static const char* testSceneName[NUM_TESTS] = { "Dialog", 
													"Render", 
													"Color", 
													"Particle", 
													"Nav", "Nav2", 
													"Battle", 
													"Animation", 
													"Asset\nPreview",
													"Sound",
													"Fluid"
	};

	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		testScene[i].SetText( testSceneName[i] );
	}

	static const char* gameSceneName[NUM_GAME] = { "Generate World", "Continue" };
	for( int i=0; i<NUM_GAME; ++i ) {
		gameScene[i].Init( &gamui2D, lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD ) );
		gameScene[i].SetText( gameSceneName[i] );
	}

	note.Init( &gamui2D );
	note.SetText("Thanks for playing this beta of Altera! (" VERSION "). "
				 "Please see the README.txt for information and a link to the 'how to play' wiki.");
	//You can now toggle(in the upper right on this screen) between Mouse + Keyboard or Touch controls." );

	audioButton.Init(&gamui2D, lumosGame->GetButtonLook(LumosGame::BUTTON_LOOK_STD));
	SetAudioButton();

	mouseTouchButton.Init(&gamui2D, lumosGame->GetButtonLook(LumosGame::BUTTON_LOOK_STD));
	SetMouseTouchButton();

	RenderAtom gray = LumosGame::CalcPaletteAtom(PAL_GRAY * 2, PAL_GRAY);
	testCanvas.Init(&gamui2D, gray);
	testCanvas.DrawRectangle(0, 0, 100, 100);
	//testCanvas.SetVisible(false);

	creditsButton.Init(&gamui2D, lumosGame->GetButtonLook(0));
	creditsButton.SetText("Credits");

	testMap = new TestMap(64,62);
	const Game::Palette* palette = game->GetPalette();
	Color3F c = palette->Get3F(0, 6);
	testMap->SetColor(c);

	engine = new Engine(&screenport, game->GetDatabase(), testMap);
	engine->LoadConfigFiles("./res/particles.xml", "./res/lighting.xml");

	engine->lighting.direction.Set(0.3f, 1, 1);
	engine->lighting.direction.Normalize();
	SpaceTree* tree = engine->GetSpaceTree();

	model[TROLL]		= new Model("troll", tree);
	model[MANTIS]		= new Model("mantis", tree);
	model[HUMAN_MALE]	= new Model("humanMale", tree);
	model[HUMAN_FEMALE] = new Model("humanFemale", tree);
	model[RED_MANTIS]	= new Model("redmantis", tree);
	model[CYCLOPS]		= new Model("cyclops", tree);
	//temple				= engine->AllocModel("pyramid0");

	Vector3F forward = { 0, 0, 1 };
	Vector3F across = { 0.4f, 0, 0 };

//	static const float STEP = 0.4f;
	static const float ANGLE = 5.f;
	for (int i = 0; i < NUM_MODELS; ++i) {
		Vector3F v = { 30.5f, 0, 39.5f };
		v = v + float(i) * across;

		//if (i == CYCLOPS) {
		//	v = v - forward;
		//}
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
		const int team = Team::CombineID(TEAM_HOUSE, 18);
		HumanGen gen(true, 99, team, false);
		ProcRenderInfo info;
		gen.AssignSuit(&info);
		model[HUMAN_FEMALE]->SetTextureXForm(info.te.uvXForm);
		model[HUMAN_FEMALE]->SetColorMap(info.color);
	}
	{
		const int team = Team::CombineID(TEAM_HOUSE, 19);
		HumanGen gen(false, 1, team, false);
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


void TitleScene::DeleteEngine()
{
	for (int i = 0; i < NUM_MODELS; ++i) {
		delete model[i];
		model[i] = 0;
	}
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

	float aspect = gamui2D.AspectRatio();
	float factor = 0.5;
	if ( aspect >= 0.5f ) {
		background.SetSize(gamui2D.Width()*factor, gamui2D.Width()*0.5f*factor);
	}
	else {
		background.SetSize(gamui2D.Height()*2.0f*factor, gamui2D.Height()*factor);
	}
	background.SetCenterPos(gamui2D.Width()*0.5f, gamui2D.Height()*0.5f*factor);

	bool visible = game->GetDebugUI();
	LayoutCalculator layout = DefaultLayout();
	int c = 0;
	for( int i=0; i<NUM_TESTS; ++i ) {
		testScene[i].SetVisible( visible );

		int y = c / TESTS_PER_ROW;
		int x = c - y*TESTS_PER_ROW;
		layout.PosAbs( &testScene[i], x, y );
		++c;
	}
	layout.SetSize( LAYOUT_SIZE_X, LAYOUT_SIZE_Y );

	layout.PosAbs(&gameScene[GENERATE_WORLD], -2, -1, 2, 1);
	layout.PosAbs(&gameScene[CONTINUE], 0, -1, 2, 1);

	gameScene[CONTINUE].SetEnabled( false );
//	const char* datPath = game->GamePath( "game", 0, "dat" );
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

	layout.PosAbs( &note, 0, -2 );
	note.SetVisible(!visible);
	note.SetBounds( gamui2D.Width() -(layout.GutterX() * 2.0f), 0 );

	layout.PosAbs(&audioButton, -1, 0);
	layout.PosAbs(&mouseTouchButton, -2, 0);
	layout.PosAbs(&creditsButton, -3, 0);

	testCanvas.SetPos(gamui2D.Width() - 100, gamui2D.Height() - 100);
	testCanvas.SetVisible(false);
}


void TitleScene::SetAudioButton()
{
	if (SettingsManager::Instance()->AudioOn()) {
		audioButton.SetDeco(lumosGame->CalcUIIconAtom("audioOn", true), lumosGame->CalcUIIconAtom("audioOn", false));
		audioButton.SetDown();
	}
	else {
		audioButton.SetDeco(lumosGame->CalcUIIconAtom("audioOff", true), lumosGame->CalcUIIconAtom("audioOff", false));
		audioButton.SetUp();
	}
}


void TitleScene::SetMouseTouchButton()
{
	if (SettingsManager::Instance()->TouchOn()) {
		mouseTouchButton.SetText("Touch");
		mouseTouchButton.SetDown();
	}
	else {
		mouseTouchButton.SetText("Keyboard\n&Mouse");
		mouseTouchButton.SetUp();
	}
}


void TitleScene::ItemTapped( const gamui::UIItem* item )
{
	XenoAudio::Instance()->SetAudio(SettingsManager::Instance()->AudioOn());
	if ( item == &testScene[TEST_DIALOG] ) {
		game->PushScene( LumosGame::SCENE_DIALOG, 0 );
	}
	else if ( item == &testScene[TEST_RENDER] ) {
		game->PushScene( LumosGame::SCENE_RENDERTEST, new RenderTestSceneData( 0 ) );
	}
	else if ( item == &testScene[TEST_COLOR] ) {
		game->PushScene( LumosGame::SCENE_COLORTEST, 0 );
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
	else if (item == &testScene[TEST_FLUID]) {
		game->PushScene(LumosGame::SCENE_FLUID, 0);
	}
	else if ( item == &gameScene[GENERATE_WORLD] ) {
		game->PushScene( LumosGame::SCENE_WORLDGEN, 0 );
	}
	else if ( item == &gameScene[CONTINUE] ) {
		game->PushScene( LumosGame::SCENE_GAME, 0 );
	}
	else if (item == &audioButton) {
		SettingsManager::Instance()->SetAudioOn(audioButton.Down());
		SetAudioButton();
	}
	else if (item == &mouseTouchButton) {
		SettingsManager::Instance()->SetTouchOn(mouseTouchButton.Down());
		SetMouseTouchButton();
	}
	else if (item == &creditsButton) {
		game->PushScene(LumosGame::SCENE_CREDITS, 0);
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
	if (key == GAME_HK_DEBUG_ACTION) {
		seed++;
#if 1
		GLOUTPUT(("Seed=%d\n", seed));
		for (int i = HUMAN_FEMALE; i <= HUMAN_MALE; ++i) {
			if (model[i]) {
				HumanGen gen(true, seed + i - HUMAN_FEMALE, TEAM_HOUSE, false);
				ProcRenderInfo info;
				gen.AssignSuit(&info);
				model[i]->SetTextureXForm(info.te.uvXForm);
				model[i]->SetColorMap(info.color);
			}
		}
#else
		model[0]->SetYRotation(0);

		const ModelResource* res = model[0]->GetResource();
		Vector3F head = { 0, 0, 0 };
		for (int i = 0; i < EL_MAX_BONES; ++i) {
			if (res->header.modelBoneName[i] == "head") {
				head = res->boneCentroid[i];
				break;
			}
		}
		Matrix4 xform = model[0]->XForm();
		Vector3F local = xform * head;

		float x = model[0]->Pos().x;

		const Vector3F CAM    = { local.x, local.y, local.z + 3.0f };
		const Vector3F TARGET = { local.x, local.y, local.z };

		engine->CameraLookAt(CAM, TARGET);
#endif
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
