#include "creditsscene.h"
#include "../game/lumosgame.h"

using namespace gamui;

CreditsScene::CreditsScene(LumosGame* game) : Scene(game)
{
	InitStd(&gamui2D, &okay, 0);
	text.Init(&gamui2D);

	text.SetText(
		"Altera Orbis\n"
		"Grinning Lizard Software\n"
		"\n"
		"Game Design and Programming\n"
			"\tLee Thomason\n"
		"\n"
		"Concept Art\n"
			"\tShroomArts\n"
		"\n"
		"3D Art\n"
			"\tLee Thomason, ShroomArts\n"
		"\n"
		"2D Art\n"
			"\tShroomArts, Lee Thomason\n"
		"\n"
		"Sound\n"
			"\tJustin Prymowicz\n"
		"\n"
		"Additional Game Design\n"
			"\tShroomArts, Jeff Mott\n"
		"\n"
		"Additional Concept Art\n"
			"\tJohan Forseberg\n"
		"\n"
		"\n"
		"Grinning Lizard:\t\tgrinninglizard.com/altera\n"
		"ShroomArts:\t\tshroomarts.blogspot.com\n"
		"Justin Prymowicz:\t\tjustinprymowicz.com\n"
		);
}


CreditsScene::~CreditsScene()
{
}


void CreditsScene::Resize()
{
	const Screenport& port = game->GetScreenport();
	PositionStd(&okay, 0);
	LayoutCalculator layout = DefaultLayout();

	float offset = layout.GutterX() + layout.Width();
	text.SetPos(offset, layout.GutterY() + layout.Height());
	text.SetBounds(gamui2D.Width() - offset*2.0f, 0);

	text.SetTab(layout.Width()*1.0f);
}


void CreditsScene::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		game->PopScene();
	}
}


grinliz::Color4F CreditsScene::ClearColor()
{
	const Game::Palette* palette = game->GetPalette();
	return palette->Get4F(0, 5);
}

