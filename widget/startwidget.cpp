#include "startwidget.h"
#include "../game/lumosgame.h"

using namespace gamui;

static const float GUTTER = 5.0f;

StartGameWidget::StartGameWidget()
{
}


StartGameWidget::~StartGameWidget()
{
}

void StartGameWidget::Init(Gamui* gamui, const ButtonLook& look, const LayoutCalculator& calc)
{
	calculator = calc;

	gamui->StartDialog("StartGameWidget");
	background.Init(gamui, LumosGame::CalcUIIconAtom("background0"), false);
	background.SetSize(200, 200);
	background.SetSlice(true);

	topLabel.Init(gamui);
	bodyLabel.Init(gamui);
	countLabel.Init(gamui);
	topLabel.SetText("MotherCore has granted you access to a neutral domain core. Choose wisely.");
	bodyLabel.SetText("Foo-45\nTemperature: warm Humidty: low\n38% Land 62% water 19% Flora nPorts");
	countLabel.SetText("1/1");

	/*
	primaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);
	secondaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);

	prevColor.Init(gamui, look);
	nextColor.Init(gamui, look);
	*/
	prevDomain.Init(gamui, look);
	nextDomain.Init(gamui, look);
	/*
	prevColor.SetText("<");
	nextColor.SetText(">");
	*/
	prevDomain.SetText("<");
	nextDomain.SetText(">");

	okay.Init(gamui, look);
	okay.SetDeco(LumosGame::CalcUIIconAtom("okay", true), LumosGame::CalcUIIconAtom("okay", false));

	textHeight = gamui->GetTextHeight();
	gamui->EndDialog();
}


void StartGameWidget::SetPos(float x, float y)
{
	background.SetPos(x, y);
	calculator.SetOffset(x, y);

	calculator.PosAbs(&topLabel, 0, 0);

	/*
	calculator.PosAbs(&prevColor, 0, 1);
	calculator.PosAbs(&primaryColor, 1, 1);
	calculator.PosAbs(&secondaryColor, 2, 1);
	calculator.PosAbs(&nextColor, 3, 1);
	*/
	calculator.PosAbs(&bodyLabel, 0, 1);
	
	// hack to drop the bottom a bit.
	calculator.SetOffset(x, y + textHeight);

	calculator.PosAbs(&okay, 0, 3);
	calculator.PosAbs(&prevDomain, 1, 3);
	calculator.PosAbs(&countLabel, 2, 3);
	calculator.PosAbs(&nextDomain, 3, 3);

	topLabel.SetBounds(background.Width() - GUTTER*2.0f, 0);
	bodyLabel.SetBounds(background.Width() - GUTTER*2.0f, 0);
}


void StartGameWidget::SetSize(float width, float h)
{
	background.SetSize(width, h);
	// Calls SetPos(), but not vice-versa
	SetPos(background.X(), background.Y());
}


void StartGameWidget::SetVisible(bool vis)
{
	background.SetVisible(vis);
	topLabel.SetVisible(vis);
	bodyLabel.SetVisible(vis);
	countLabel.SetVisible(vis);
	prevDomain.SetVisible(vis);
	nextDomain.SetVisible(vis);
	okay.SetVisible(vis);
}
