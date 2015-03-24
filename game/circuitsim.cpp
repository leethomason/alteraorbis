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

//	if (wg.Circuit() == CIRCUIT_SWITCH) {
//	}
}


void CircuitSim::TriggerDetector(const grinliz::Vector2I& pos)
{
	WorldMap* worldMap = context->worldMap;
	const WorldGrid& wg = context->worldMap->grid[worldMap->INDEX(pos)];

//	if (wg.Circuit() == CIRCUIT_DETECT_ENEMY ) {
//	}
}


void CircuitSim::DoTick(U32 delta)
{
	Vector2I sector = context->chitBag->GetHomeSector();
	if (!sector.IsZero()) {
		DrawGroups(sector, &canvas);
	}
}


void CircuitSim::FillGroup(Group* g, const Vector2I& pos, Chit* chit)
{
	g->bounds.DoUnion(pos);
	g->idArr.PushIfCap(chit->ID());
	hashTable.Remove(pos);
	for (int i = 0; i < 4; ++i) {
		Vector2I nextPos = pos.Adjacent(i);
		Chit* nextChit = 0;
		if (hashTable.Query(nextPos, &nextChit)) {
			FillGroup(g, nextPos, nextChit);
		}
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

	hashTable.Clear();
	for (Chit* chit : queryArr) {
		Vector2I pos = ToWorld2I(chit->Position());
		hashTable.Add(pos, chit);
	}

	for (Chit* chit : queryArr) {
		// If it is still in the hash table, then
		// it is a group.
		Vector2I pos = ToWorld2I(chit->Position());
		if (hashTable.Query(pos, 0)) {
			Group* g = groups[DEVICE_GROUP].PushArr(1);
			g->bounds.Set(pos.x, pos.y, pos.x, pos.y);
			FillGroup(g, pos, chit);
		}
	}

	canvas->Clear();
	for (const Group& group : groups[DEVICE_GROUP]) {
		static float thickness = 1.0f / 32.0f;
		static float gutter = thickness;
		static float half = thickness * 0.5f;
		static float arc = 1.0f / 16.0f;

		float x0 = float(group.bounds.min.x) + half + gutter;
		float y0 = float(group.bounds.min.y) + half + gutter;
		float x1 = float(group.bounds.max.x + 1) - half - gutter;
		float y1 = float(group.bounds.max.y + 1) - half - gutter;

		canvas->DrawRectangleOutline(x0, y0, x1 - x0, y1 - y0, thickness, arc);
	}
}

