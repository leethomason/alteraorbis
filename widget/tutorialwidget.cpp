#include "tutorialwidget.h"
#include "../game/lumosgame.h"
#include "../script/procedural.h"

using namespace grinliz;

TutorialWidget::TutorialWidget()
{
	currentTutorial = 0;
}


TutorialWidget::~TutorialWidget()
{
}


void TutorialWidget::Init(gamui::Gamui* gamui, const gamui::ButtonLook& look, const gamui::LayoutCalculator& calculator)
{
	this->calculator = calculator;
	background.Init(gamui, LumosGame::CalcUIIconAtom("background0"), false);
	background.SetSlice(true);

	adviser.Init(gamui, LumosGame::CalcUIIconAtom("adviser"), true);
	next.Init(gamui, look);
	done.Init(gamui, look);
	textLabel.Init(gamui);
	progressLabel.Init(gamui);

	next.SetText("Next");
	done.SetText("Close");

	canvas.Init(gamui, LumosGame::CalcPaletteAtom(1, PAL_TANGERINE));
	canvas.SetLevel(gamui::Gamui::LEVEL_TEXT - 1);

	DoLayout();
}


void TutorialWidget::SetVisible(bool v)
{
	background.SetVisible(v);
	next.SetVisible(v);
	done.SetVisible(v);
	textLabel.SetVisible(v);
	adviser.SetVisible(v);
	canvas.SetVisible(v);
	progressLabel.SetVisible(v);
}


void TutorialWidget::DoLayout()
{
	static const int X = 2;
	static const int Y = 2;
	static const int W = 7;
	static const int H = 6;
	static const int ADV = 2;

	calculator.PosAbs(&background, X, Y, W, H);
	calculator.PosAbs(&adviser, X, Y + 1, ADV, ADV);
	calculator.PosAbs(&done, X, Y + H - 1);
	calculator.PosAbs(&next, X + W - 1, Y + H - 1);
	calculator.PosAbs(&textLabel, X+ADV, Y, W-1, H-1);
	calculator.PosAbs(&progressLabel, X + 2, Y + H - 1);

	adviser.SetSize(adviser.Width(), adviser.Width());
	textLabel.SetPos(adviser.X() + adviser.Width() + calculator.GutterX(), adviser.Y());
	textLabel.SetBounds((background.X() + background.Width()) - (adviser.X() + adviser.Width()) - calculator.GutterX() * 2.0f, 0);
}


void TutorialWidget::Add(const gamui::IWidget* item, const char* text)
{
	Tutorial tut = { item, text };
	tutorials.Push(tut);
	if (tutorials.Size() == 1) {
		currentTutorial = 0;
		ShowTutorial();
	}
}


void TutorialWidget::ShowTutorial()
{
	textLabel.SetText(tutorials[currentTutorial].text);

	canvas.Clear();
	const gamui::IWidget* uiItem = tutorials[currentTutorial].item;

	CStr<32> str;
	str.Format("%d/%d", currentTutorial + 1, tutorials.Size());
	progressLabel.SetText(str.safe_str());

	if (uiItem) {
		Vector2F dest = { uiItem->X() + uiItem->Width() / 2, uiItem->Y() + uiItem->Height() / 2 };
		Vector2F origin = { 0, 0 };
		origin.x =  (dest.x < adviser.X()) ? adviser.X() : adviser.X() + adviser.Width();
		origin.y =  (dest.y < adviser.Y()) ? adviser.Y() : adviser.Y() + adviser.Height();
		// Upper right never looks good:
		if (dest.x > adviser.X() && dest.y < adviser.Y())
			origin.Set(adviser.X(), adviser.Y());

//		static const float LINE = 6;
		static const float RECTANGLE = 20;
		canvas.DrawLine(origin.x, origin.y, dest.x, dest.y, 8.0f);
		canvas.DrawRectangle(origin.x - RECTANGLE / 2, origin.y - RECTANGLE / 2, RECTANGLE, RECTANGLE);
		canvas.DrawRectangle(dest.x - RECTANGLE / 2, dest.y - RECTANGLE / 2, RECTANGLE, RECTANGLE);
	}
}


void TutorialWidget::ItemTapped(const gamui::UIItem* item)
{
	if (item == &next) {
		currentTutorial++;
		if (currentTutorial >= tutorials.Size()) {
			SetVisible(false);
		}
		else {
			ShowTutorial();
		}
	}
	else if (item == &done) {
		SetVisible(false);
	}
}
