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

	background.Init(gamui, LumosGame::CalcUIIconAtom("background0"), false);
	background.SetSize(200, 200);
	background.SetSlice(true);

	topLabel.Init(gamui);
	bodyLabel.Init(gamui);
	topLabel.SetText("MotherCore has granted you access to a neutral domain core. Choose wisely.");
	bodyLabel.SetText("Foo-45\nTemperature: warm Humidty: low\n38% Land 62% water 19% Flora");

	primaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);
	secondaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);

	nextColor.Init(gamui, look);
	prevDomain.Init(gamui, look);
	nextDomain.Init(gamui, look);

	nextColor.SetText(">");
	prevDomain.SetText("<");
	nextDomain.SetText(">");

	textHeight = gamui->GetTextHeight();
}


void StartGameWidget::SetPos(float x, float y)
{
	background.SetPos(x, y);
	calculator.SetOffset(x, y);

	calculator.PosAbs(&topLabel, 0, 0);
	calculator.PosAbs(&primaryColor, 0, 1);
	calculator.PosAbs(&secondaryColor, 1, 1);
	calculator.PosAbs(&nextColor, 2, 1);
	calculator.PosAbs(&bodyLabel, 0, 2);
	calculator.PosAbs(&prevDomain, 0, 3);
	calculator.PosAbs(&nextDomain, 2, 3);

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
}



