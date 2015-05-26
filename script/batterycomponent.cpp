#include "batterycomponent.h"
#include "../xegame/chitbag.h"
#include "../game/worldmap.h"
#include "../game/circuitsim.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitcontext.h"
#include "../engine/map.h"
#include "../game/lumosgame.h"
#include "../script/procedural.h"

using namespace grinliz;
using namespace gamui;

static const float YSIZE = 0.2f;

void BatteryComponent::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	XARC_SER(xs, charge);
	this->EndSerialize(xs);
}

void BatteryComponent::OnAdd(Chit* chit, bool initialize)
{
	super::OnAdd(chit, initialize);

	RenderAtom higher = LumosGame::CalcPaletteAtom(PAL_GRAY * 2, PAL_GRAY);
	RenderAtom lower = LumosGame::CalcPaletteAtom(PAL_GREEN * 2, PAL_GREEN);

	higher.renderState = (const void*) Map::RENDERSTATE_MAP_TRANSLUCENT;
	lower.renderState = (const void*) Map::RENDERSTATE_MAP_TRANSLUCENT;

	bar.Init(&Context()->worldMap->overlay0, lower, higher);
	bar.SetSize(2.0f, YSIZE);
	bar.SetPos(chit->Position().x - 1.0f, chit->Position().z + (1.0f - YSIZE));
}


void BatteryComponent::OnRemove()
{
	super::OnRemove();
}


bool BatteryComponent::UseCharge()
{
	if (charge >= 1.0f) {
		charge -= 1.0f;
		return true;
	}
	return false;
}


int BatteryComponent::DoTick(U32 delta)
{
	charge += Travel(0.5f, delta);
	if (charge > 4) charge = 4;

	bar.SetRange(charge / 4.0f);
	return MAX_FRAME_TIME;
}


void BatteryComponent::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (chit == ParentChit() && msg.ID() == ChitMsg::CHIT_POS_CHANGE) {
		bar.SetPos(chit->Position().x - 1.0f, chit->Position().z + (1.0f - YSIZE));
	}
}
