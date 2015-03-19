#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/particle.h"

#include "../script/battlemechanics.h"
#include "../script/batterycomponent.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/game.h"
#include "../audio/xenoaudio.h"

#include "../Shiny/include/Shiny.h"

using namespace grinliz;

CircuitSim::CircuitSim(const ChitContext* _context) : context(_context)
{
	GLASSERT(context);
}


CircuitSim::~CircuitSim()
{
}

void CircuitSim::Serialize(XStream* xs)
{
	XarcOpen(xs, "CircuitSim");
	XarcClose(xs);
}


void CircuitSim::TriggerSwitch(const grinliz::Vector2I& pos)
{
	WorldMap* worldMap = context->worldMap;
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos)];

	if (wg.Circuit() == CIRCUIT_SWITCH) {
	}
}


void CircuitSim::TriggerDetector(const grinliz::Vector2I& pos)
{
	WorldMap* worldMap = context->worldMap;
	const WorldGrid& wg = context->worldMap->grid[worldMap->INDEX(pos)];

	if (wg.Circuit() == CIRCUIT_DETECT_ENEMY ) {
	}
}


void CircuitSim::DoTick(U32 delta)
{

}


void CircuitSim::DrawGroups(const grinliz::Vector2I& sector, gamui::Canvas* canvas)
{
	// Power:
	// - temples

	// Sensors:
	// - detectors
	// - switch

	// Devices:
	// - turrets

	const int group = DEVICE_GROUP;
	context->chitBag->FindBuilding(ISC::turret, sector, 0, LumosChitBag::EFindMode::NEAREST, &queryArr, 0);

	bool found = false;
	for (int id : groupArr[group].idArr) {
		Chit* chit = context->chitBag->GetChit(id);
	}
}

