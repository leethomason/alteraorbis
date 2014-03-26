#include "soundscene.h"
#include "../game/lumosgame.h"
#include "../audio/xenoaudio.h"

using namespace gamui;
using namespace grinliz;

SoundScene::SoundScene(LumosGame* game) : Scene(game)
{
	lumosGame = game;
	game->InitStd(&gamui2D, &okay, 0);

	for (int i = 0; i < NUM_TESTS; ++i) {
		test[i].Init(&gamui2D, lumosGame->GetButtonLook(0));
		const char* NAME[NUM_TESTS] = { "Basic", "Left", "Right", "RotLeft", "RotRight" };
		test[i].SetText(NAME[i]);
	}
	facing.Set(0,0,1);
}


SoundScene::~SoundScene()
{
}


void SoundScene::Resize()
{
	lumosGame->PositionStd(&okay, 0);
	LayoutCalculator layout = lumosGame->DefaultLayout();
	for (int i = 0; i < NUM_TESTS; ++i) {
		layout.PosAbs(&test[i], i, 0);
	}
}


void SoundScene::DoTick(U32 delta)
{
	XenoAudio::Instance()->SetListener(V3F_ZERO, facing);
}


void SoundScene::ItemTapped(const gamui::UIItem* item)
{
	/* remember the coordinate system - how many times
	   has this bitten me?
	
		+----- x
		|
		|
		z
		(y up)
	*/

	if (item == &okay) {
		game->PopScene();
	}
	else if (item == &test[BASIC_TEST]) {
		XenoAudio::Instance()->Play("testLaser", 0);
	}
	else if (item == &test[LEFT_TEST]) {
		Vector3F pos = { -10, 0, 0 };
		facing.Set(0, 0, -1);
		XenoAudio::Instance()->Play("testLaser", &pos);
	}
	else if (item == &test[RIGHT_TEST]) {
		Vector3F pos = { 10, 0, 0 };
		facing.Set(0, 0, -1);
		XenoAudio::Instance()->Play("testLaser", &pos);
	}
	else if (item == &test[ROT_LEFT_TEST]) {
		Vector3F pos = { -7, 0, -7 };
		facing.Set(1, 0, -1);
		XenoAudio::Instance()->Play("testLaser", &pos);
	}
	else if (item == &test[ROT_RIGHT_TEST]) {
		Vector3F pos = { 7, 0, 7 };
		facing.Set(1, 0, -1);
		XenoAudio::Instance()->Play("testLaser", &pos);
	}
}



