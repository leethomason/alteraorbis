#include "soundscene.h"
#include "../game/lumosgame.h"
#include "../audio/xenoaudio.h"

using namespace gamui;
using namespace grinliz;

SoundScene::SoundScene(LumosGame* game) : Scene(game)
{
	lumosGame = game;
	game->InitStd(&gamui2D, &okay, 0);

	basicTest.Init(&gamui2D, lumosGame->GetButtonLook(0));
	basicTest.SetText("Basic");
}


SoundScene::~SoundScene()
{
}


void SoundScene::Resize()
{
	lumosGame->PositionStd(&okay, 0);
	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs(&basicTest, 0, 0);
}


void SoundScene::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		game->PopScene();
	}
	else if (item == &basicTest) {
		XenoAudio::Instance()->Play("testLaser");
	}
}



