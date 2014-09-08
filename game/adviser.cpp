#include "adviser.h"

#include "../script/buildscript.h"
#include "../script/corescript.h"

#include "../grinliz/glstringutil.h"

#include "../game/wallet.h"
#include "../game/gameitem.h"

using namespace grinliz;
using namespace gamui;

Adviser::Adviser()
{
	text = 0;
	image = 0;
}


void Adviser::Attach(gamui::TextLabel *_text, gamui::Image* _image)
{
	text = _text;
	image = _image;
}


void Adviser::DoTick(int time, CoreScript* cs, int nWorkers, const int* buildCounts, int nBuildCounts)
{
	static const int BUILD_ADVISOR[] = {
		BuildScript::FARM,
		BuildScript::SLEEPTUBE,
		BuildScript::DISTILLERY,
		BuildScript::BAR,
		BuildScript::MARKET,
		BuildScript::FORGE,
		BuildScript::TEMPLE,
		BuildScript::GUARDPOST,
		BuildScript::EXCHANGE,
		BuildScript::KIOSK_N,
		BuildScript::KIOSK_M,
		BuildScript::KIOSK_C,
		BuildScript::KIOSK_S,
		BuildScript::VAULT,
		BuildScript::CIRCUITFAB
	};

	static const int NUM_BUILD_ADVISORS = GL_C_ARRAY_SIZE(BUILD_ADVISOR);
	CStr<100> str = "";

	if (cs) {
		if (nWorkers == 0) {
			str.Format("Build a worker bot.");
		}
		else {
			const Wallet& wallet = cs->ParentChit()->GetItem()->wallet;

			for (int i = 0; i < NUM_BUILD_ADVISORS; ++i) {
				int id = BUILD_ADVISOR[i];
				if (id < nBuildCounts && buildCounts[id] == 0) {
					BuildScript buildScript;
					const BuildData& data = buildScript.GetData(id);

					if (wallet.gold >= data.cost) {
						str.Format("Advisor: Build a %s.", data.cName);
					}
					else {
						str.Format("Advisor: Collect gold by defeating attackers\nor raiding domains.");
					}
					break;
				}
			}
		}
	}
	text->SetText(str.c_str());
}

