#include "adviser.h"

#include "../script/buildscript.h"
#include "../script/corescript.h"

#include "../grinliz/glstringutil.h"

#include "../game/wallet.h"
#include "../game/gameitem.h"
#include "../game/lumosgame.h"

#include "../engine/uirendering.h"

using namespace grinliz;
using namespace gamui;

Adviser::Adviser()
{
	text = 0;
	image = 0;
	state = 0;
	timer = 0;
}


void Adviser::Attach(gamui::TextLabel *_text, gamui::Image* _image)
{
	text = _text;
	image = _image;

	RenderAtom atom = LumosGame::CalcUIIconAtom("adviser", true);
	atom.renderState = (void*)UIRenderer::RENDERSTATE_UI_NORMAL;
	image->SetAtom(atom);

	image->SetVisible(false);
	text->SetVisible(false);
	state = DORMANT;
	timer = 0;
}


void Adviser::DoTick(int delta, CoreScript* cs, int nWorkers, const int* buildCounts, int nBuildCounts)
{
	delta = Max(delta, (int)MAX_FRAME_TIME);
	timer += delta;

	if (state == DORMANT && timer > DORMANT_TIME) {
		state = GREETING;
		image->SetRotationY(90.0f);
		rotation = 90.0f;
		image->SetVisible(true);
		text->SetVisible(true);
		text->SetText("Greetings, Core. I am The Adviser.\nMay our domain long stand.");
		timer = 0;
		return;
	}
	else if (state == GREETING) {
		rotation -= Travel(180.0f, float(delta) / 1000.0f);
		if (rotation < 0) rotation = 0;
		image->SetRotationY(rotation);
		if (rotation == 0 && timer > GREETING_TIME) {
			timer = 0;
			text->SetText("");
			state = ADVISING;
			rotation = 0;
		}
		return;
	}

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
			str.Format("Build at least one worker bot.\nThey construct and maintain.");
		}
		else {
			const Wallet& wallet = cs->ParentChit()->GetItem()->wallet;

			for (int i = 0; i < NUM_BUILD_ADVISORS; ++i) {
				int id = BUILD_ADVISOR[i];
				if (id < nBuildCounts && buildCounts[id] == 0) {
					BuildScript buildScript;
					const BuildData& data = buildScript.GetData(id);

					if (wallet.gold >= data.cost) {
						if (id >= BuildScript::KIOSK_N && id <= BuildScript::KIOSK_S) {
							str.Format("Build a %s kiosk to attract Visitors.", data.cName);
						}
						else if (id == BuildScript::TEMPLE) {
							str.Format("Building a Temple increases our potential.\nAnd attracts monsters.");
						}
						else {
							//str.Format("A(n) %s is recommended.", data.cName);
							str.Format("Recommendation: %s.", data.cName);
						}
					}
					else {
						str.Format("We need Au. Collect Au by defeating attackers\nor raiding domains.");
					}
					break;
				}
			}
		}
	}
	text->SetText(str.c_str());

	if (str.empty()) {
		if (rotation < 90.0f) {
			rotation += Travel(180.0f, float(delta) / 1000.0f);
			if (rotation > 90.0f) rotation = 90.0f;
		}
	}
	else {
		if (rotation > 0) {
			rotation -= Travel(180.0f, float(delta) / 1000.0f);
			if (rotation < 0) rotation = 0;
		}
	}
	image->SetRotationY(rotation);
	image->SetVisible(rotation != 90.0f);
}


