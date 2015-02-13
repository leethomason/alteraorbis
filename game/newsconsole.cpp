#include "newsconsole.h"
#include "../widget/consolewidget.h"
#include "news.h"
#include "../xegame/spatialcomponent.h"
#include "../script/corescript.h"
#include "lumoschitbag.h"
#include "lumosgame.h"

using namespace gamui;
using namespace grinliz;

NewsConsole::NewsConsole()
{
	currentNews = 0;
}


NewsConsole::~NewsConsole()
{
}


void NewsConsole::Init(Gamui* gamui2D, LumosChitBag* cb)
{
	chitBag = cb;
	consoleWidget.Init(gamui2D);
}


void NewsConsole::DoTick(int time, CoreScript* homeCore)
{
	consoleWidget.DoTick(time);
	ProcessNewsToConsole(homeCore);
}


void NewsConsole::ProcessNewsToConsole(CoreScript* homeCore)
{
	NewsHistory* history = chitBag->GetNewsHistory();
	currentNews = Max(currentNews, history->NumNews() - 40);
	GLString str;
	Vector2I homeSector = { 0, 0 };
	if (homeCore) {
		homeSector = ToSector(homeCore->ParentChit()->Position());
	}

	// Check if news sector is 1)current avatar sector, or 2)domain sector

	RenderAtom atom;
	Vector2F pos2 = { 0, 0 };

	for (; currentNews < history->NumNews(); ++currentNews) {
		const NewsEvent& ne = history->News(currentNews);
		Vector2I sector = ne.Sector();
		Vector2F pos2 = ne.Pos();

		str = "";

		switch (ne.What()) {
			case NewsEvent::DENIZEN_CREATED:
			case NewsEvent::ROGUE_DENIZEN_JOINS_TEAM:
			if (homeCore && homeCore->IsCitizenItemID(ne.FirstItemID())) {
				ne.Console(&str, chitBag, 0);
				atom = LumosGame::CalcUIIconAtom("greeninfo");
			}
			break;

			case NewsEvent::DENIZEN_KILLED:
			case NewsEvent::STARVATION:
			case NewsEvent::BLOOD_RAGE:
			case NewsEvent::VISION_QUEST:
			if (homeCore && homeCore->IsCitizenItemID(ne.FirstItemID())) {
				ne.Console(&str, chitBag, 0);
				atom = LumosGame::CalcUIIconAtom("warning");
			}
			break;

			case NewsEvent::FORGED:
			case NewsEvent::UN_FORGED:
			case NewsEvent::PURCHASED:
			if ((homeCore && homeCore->IsCitizenItemID(ne.FirstItemID()))
				|| sector == homeSector)
			{
				ne.Console(&str, chitBag, 0);
				atom = LumosGame::CalcUIIconAtom("greeninfo");
			}
			break;

			case NewsEvent::GREATER_MOB_CREATED:
			case NewsEvent::GREATER_MOB_KILLED:
			ne.Console(&str, chitBag, 0);
			atom = LumosGame::CalcUIIconAtom("greeninfo");
			break;

			case NewsEvent::DOMAIN_CREATED:
			case NewsEvent::DOMAIN_DESTROYED:
			case NewsEvent::GREATER_SUMMON_TECH:
			case NewsEvent::DOMAIN_CONQUER:
			ne.Console(&str, chitBag, 0);
			break;

			default:
			break;
		}
		if (!str.empty()) {
			consoleWidget.Push(str, atom, pos2);
		}
	}
}

