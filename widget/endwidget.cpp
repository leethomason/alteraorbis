#include "endwidget.h"
#include "../game/lumosgame.h"
#include "../game/worldinfo.h"
#include "../game/weather.h"
#include "../game/lumosmath.h"
#include "../game/lumoschitbag.h"

#include "../engine/engine.h"

#include "../xegame/cameracomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/scene.h"

#include "../script/plantscript.h"

using namespace gamui;
using namespace grinliz;

static const float GUTTER = 5.0f;

EndGameWidget::EndGameWidget()
{
	engine = 0;
	chitBag = 0;
	iScene = 0;
	history = 0;
	itemID = 0;
}


EndGameWidget::~EndGameWidget()
{
}


void EndGameWidget::SetData(NewsHistory* h, Scene* scene, IString name, int id, const CoreAchievement& a)
{
	iScene = scene;
	achievement = a;
	coreName = name;
	history = h;
	itemID = id;
	SetBodyText();
}


void EndGameWidget::Init(Gamui* gamui, const ButtonLook& look, const LayoutCalculator& calc)
{
	calculator = calc;

	gamui->StartDialog("EndGameWidget");
	background.Init(gamui, LumosGame::CalcUIIconAtom("background0"), false);
	background.SetSize(200, 200);
	background.SetSlice(true);

	bodyLabel.Init(gamui);

	okay.Init(gamui, look);
	okay.SetDeco(LumosGame::CalcUIIconAtom("okay", true), LumosGame::CalcUIIconAtom("okay", false));

	textHeight = gamui->TextHeightVirtual();
	gamui->EndDialog();
}


void EndGameWidget::SetBodyText()
{
	CStr<400> str = "--CORE DESTROYED--\n";

	str = CoreAchievement::CivTechDescription(achievement.civTechScore);

	int num;
	NewsHistory::Data data;
	history->FindItem(itemID, 0, &num, &data);
	
	str.AppendFormat(" %s has fallen.\nFounded %.2f.\nCiv-Tech score %d.\n\nAchievements: "
					 "tech=%d gold=%d population=%d.",
					 coreName.c_str(),
					 double(data.born) / double(AGE_IN_MSEC),
					 achievement.civTechScore,
					 achievement.techLevel,
					 achievement.gold,
					 achievement.population);

	bodyLabel.SetText(str.c_str());
}


void EndGameWidget::SetPos(float x, float y)
{
	background.SetPos(x, y);
	calculator.SetOffset(x, y);

	calculator.PosAbs(&bodyLabel, 0, 0);
	
	// hack to drop the bottom a bit.
	calculator.SetOffset(x, y + textHeight);

	calculator.PosAbs(&okay, 0, 3);

	bodyLabel.SetBounds(background.Width() - GUTTER*2.0f, 0);
}


void EndGameWidget::SetSize(float width, float h)
{
	background.SetSize(width, h);
	// Calls SetPos(), but not vice-versa
	SetPos(background.X(), background.Y());
}


void EndGameWidget::SetVisible(bool vis)
{
	background.SetVisible(vis);
	bodyLabel.SetVisible(vis);
	okay.SetVisible(vis);
}


void EndGameWidget::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		if (iScene) {
			iScene->DialogResult(Name(), 0);
		}
	}
}
