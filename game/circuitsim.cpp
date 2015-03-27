#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"
#include "lumosgame.h"
#include "mapspatialcomponent.h"

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
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	GLASSERT(msc);

	g->bounds.DoUnion(msc->Bounds());
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
					MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
					GLASSERT(msc);
					Group* g = groups[i].PushArr(1);
					g->bounds = msc->Bounds();
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
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				GLASSERT(msc);
				g->bounds = msc->Bounds();
				FillGroup(g, pos, chit);
			}
		}
	}
}

void CircuitSim::DrawGroups()
{
	static float thickness = 1.0f / 32.0f;
	static float thicker = thickness * 2.0f;
	static float square = thickness * 4.0f;
	static float hSquare = square * 0.5f;
	static float gutter = thickness;
	static float half = thickness * 0.5f;
	static float arc = 1.0f / 16.0f;

	for (int i = 0; i < NUM_GROUPS; ++i) {
		canvas[i].Clear();
		for (const Group& group : groups[i]) {

			float x0 = float(group.bounds.min.x) + half + gutter;
			float y0 = float(group.bounds.min.y) + half + gutter;
			float x1 = float(group.bounds.max.x + 1) - half - gutter;
			float y1 = float(group.bounds.max.y + 1) - half - gutter;

			canvas[i].DrawRectangleOutline(x0, y0, x1 - x0, y1 - y0, thickness, arc);
		}
	}

	for (const Connection& c : connections) {
		// Groups come and go - need to find the group for the connection.
		Group* groupA = 0;
		Group* groupB = 0;
		int type = 0;
		if (ConnectionValid(c.a, c.b, &type, &groupA, &groupB)) {
			Rectangle2F rectA = ToWorld2F(groupA->bounds);
			Rectangle2F rectB = ToWorld2F(groupB->bounds);

			Vector2F p0, p1;
			float len = FLT_MAX;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					Vector2F q0 = rectA.EdgeCenter(i);
					Vector2F q1 = rectB.EdgeCenter(j);
					float len2 = (q0 - q1).LengthSquared();
					if (len2 < len) {
						len = len2;
						p0 = q0;
						p1 = q1;
					}
				}
			}
			canvas[type].DrawLine(p0.x, p0.y, p1.x, p1.y, thicker);
			canvas[type].DrawRectangle(p0.x - hSquare, p0.y - hSquare, square, square);
			canvas[type].DrawRectangle(p1.x - hSquare, p1.y - hSquare, square, square);
		}
	}
}


bool CircuitSim::ConnectionValid(const Vector2I& a, const Vector2I& b, int *type, Group** groupA, Group** groupB)
{
	int indexA = -1, typeA = NUM_GROUPS;
	int indexB = -1, typeB = NUM_GROUPS;
	if (FindGroup(a, &typeA, &indexA) && FindGroup(b, &typeB, &indexB)) {
		if (   (typeA == DEVICE_GROUP && typeB != DEVICE_GROUP)
			|| (typeA != DEVICE_GROUP && typeB == DEVICE_GROUP)) 
		{
			if (type) *type = (typeA == DEVICE_GROUP) ? typeB : typeA;
			if (groupA) *groupA = &groups[typeA][indexA];
			if (groupB) *groupB = &groups[typeB][indexB];
			return true;
		}
	}
	return false;
}


void CircuitSim::Connect(const grinliz::Vector2I& a, const grinliz::Vector2I& b)
{
	int type = 0;
	if (ConnectionValid(a, b, &type, 0, 0)) {
		// If power, filter out power connection.
		Connection c = { a, b, type };
		connections.Push(c);
	}
}


bool CircuitSim::FindGroup(const grinliz::Vector2I& pos, int* groupType, int* index)
{
	for (int j = 0; j < NUM_GROUPS; ++j) {
		for (int i = 0; i < groups[j].Size(); ++i) {
			if (groups[j][i].bounds.Contains(pos)) {
				if (groupType) *groupType = j;
				if (index) *index = i;
				return true;
			}
		}
	}
	return false;
}

