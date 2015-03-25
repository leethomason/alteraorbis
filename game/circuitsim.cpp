#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"
#include "lumosgame.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"

#include "../Shiny/include/Shiny.h"

#include "../script/procedural.h"
#include "../gamui/gamui.h"

using namespace grinliz;
using namespace gamui;

CircuitSim::CircuitSim(const ChitContext* _context) : context(_context)
{
	GLASSERT(context);

	RenderAtom groupColor[NUM_GROUPS] = {
		LumosGame::CalcPaletteAtom(PAL_TANGERINE * 2, PAL_TANGERINE), // power
		LumosGame::CalcPaletteAtom(PAL_BLUE * 2, PAL_BLUE), // sensor
		LumosGame::CalcPaletteAtom(PAL_GREEN * 2, PAL_GREEN), // device
	};
	for (int i = 0; i < NUM_GROUPS; ++i) {
		canvas[i].Init(&context->worldMap->overlay0, groupColor[i]);
		canvas[i].SetLevel(-10+i);
	}
	visible = true;
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


void CircuitSim::CalcGroups(const grinliz::Vector2I& sector)
{
	IString powerNames[]  = { ISC::temple, IString() };
	bool powerGroups[] = { false, false };
	IString sensorNames[] = { ISC::detector, ISC::switchOn, ISC::switchOff, IString() };
	bool sensorGroups[] = { true, false, false, false };
	IString deviceNames[] = { ISC::turret, ISC::gate, IString() };
	bool deviceGroups[] = { true, true };

	IString* names[NUM_GROUPS] = { powerNames, sensorNames, deviceNames };
	const bool* doesGroup[NUM_GROUPS] = { powerGroups, sensorGroups, deviceGroups };

	hashTable.Clear();
	for (int i = 0; i < NUM_GROUPS; ++i) {
		groups[i].Clear();
		combinedArr.Clear();

		for (int j = 0; !names[i][j].empty(); ++j) {
			context->chitBag->FindBuilding(names[i][j], sector, 0, LumosChitBag::EFindMode::NEAREST, &queryArr, 0);
			if (doesGroup[i][j]) {
				for (Chit* chit : queryArr) {
					Vector2I pos = ToWorld2I(chit->Position());
					hashTable.Add(pos, chit);
					combinedArr.Push(chit);
				}
			}
			else {
				for (Chit* chit : queryArr) {
					Vector2I pos = ToWorld2I(chit->Position());
					Group* g = groups[i].PushArr(1);
					g->bounds.Set(pos.x, pos.y, pos.x, pos.y);
					g->idArr.Push(chit->ID());
				}
			}
		}

		for (Chit* chit : combinedArr) {
			// If it is still in the hash table, then
			// it is a group.
			Vector2I pos = ToWorld2I(chit->Position());
			if (hashTable.Query(pos, 0)) {
				Group* g = groups[i].PushArr(1);
				g->bounds.Set(pos.x, pos.y, pos.x, pos.y);
				FillGroup(g, pos, chit);
			}
		}
	}
}

void CircuitSim::DrawGroups()
{
	for (int i = 0; i < NUM_GROUPS; ++i) {
		canvas[i].Clear();
		for (const Group& group : groups[i]) {
			static float thickness = 1.0f / 32.0f;
			static float gutter = thickness;
			static float half = thickness * 0.5f;
			static float arc = 1.0f / 16.0f;

			float x0 = float(group.bounds.min.x) + half + gutter;
			float y0 = float(group.bounds.min.y) + half + gutter;
			float x1 = float(group.bounds.max.x + 1) - half - gutter;
			float y1 = float(group.bounds.max.y + 1) - half - gutter;

			canvas[i].DrawRectangleOutline(x0, y0, x1 - x0, y1 - y0, thickness, arc);
		}
	}
}

