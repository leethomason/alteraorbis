#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"
#include "lumosgame.h"

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
	canvas.Init(&context->worldMap->overlay0, LumosGame::CalcPaletteAtom(4, 2));
	canvas.SetLevel(-1);
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
	Vector2I sector = context->chitBag->GetHomeSector();
	if (!sector.IsZero()) {
		DrawGroups(sector, &canvas);
	}
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

	context->chitBag->FindBuilding(ISC::turret, sector, 0, LumosChitBag::EFindMode::NEAREST, &queryArr, 0);
	groups[DEVICE_GROUP].Clear();

	for (Chit* chit : queryArr) {
		Vector2I pos = ToWorld2I(chit->Position());

		bool found = false;
		for (int g = 0; !found && g < groups[DEVICE_GROUP].Size(); ++g) {
			Group& group = groups[DEVICE_GROUP][g];
			Rectangle2I groupBounds = group.bounds;
			groupBounds.Outset(1);
			if (groupBounds.Contains(pos)) {
				for (int id : group.idArr) {
					Chit* inChit = context->chitBag->GetChit(id);
					if (inChit) {
						Vector2I inPos = ToWorld2I(inChit->Position());
						if (pos.Adjacent(inPos, false)) {
							group.bounds.DoUnion(pos);
							group.idArr.Push(chit->ID());
							found = true;
							break;
						}
					}
				}
			}
		}
		if (!found) {
			Group* g = groups[DEVICE_GROUP].PushArr(1);
			g->bounds.Set(pos.x, pos.y, pos.x, pos.y);
			g->idArr.Push(chit->ID());
		}
	}

	canvas->Clear();
	for (const Group& group : groups[DEVICE_GROUP]) {
		canvas->DrawRectangle((float)group.bounds.min.x, (float)group.bounds.min.y, (float)group.bounds.Width(), (float)group.bounds.Height());
	}
}

