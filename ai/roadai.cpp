#include "roadai.h"
#include "../game/lumosmath.h"


using namespace grinliz;

RoadAI::RoadAI(int x, int y)
{
	offset.Set(x, y);
}

RoadAI::~RoadAI()
{}


void RoadAI::AddRoad(const grinliz::Vector2I* v, int n)
{
	for (int i = 0; i < MAX_ROADS; ++i) {
		if (road[i].Empty()) {
			Vector2I* p = road[i].PushArr(n);
			for (int k = 0; k < n; ++k) {
				p[k] = v[k];
				useMap.Set(p[k].x - offset.x, p[k].y - offset.y);
			}
			return;
		}
	}
	GLASSERT(0);	// out of roads.
}


void RoadAI::AddPlaza(const grinliz::Rectangle2I& rect)
{
	GLASSERT(plazas.HasCap());
	plazas.Push(rect);
	for (Rectangle2IIterator it(rect); !it.Done(); it.Next()) {
		useMap.Set(it.Pos().x - offset.x, it.Pos().y - offset.y);
	}
}


void RoadAI::Begin(int n)
{
	itN = n;
}


bool RoadAI::Done()
{
	for (int i = 0; i < MAX_ROADS; ++i) {
		if (itN < road[i].Size()) return false;
	}
	return true;
}


void RoadAI::Next()
{
	++itN;
}


const RoadAI::BuildZone* RoadAI::CalcZones(int* n, int size, bool _porch)
{
	int porch = _porch ? 1 : 0;
	buildZones.Clear();
	for (int i = 0; i < MAX_ROADS; ++i) {
		if (itN >= road[i].Size() || itN < 1)
			continue;

		Vector2I head = road[i][itN];
		Vector2I tail = road[i][itN - 1];

		for (int pass = 0; pass < 2; ++pass) {
			if (pass == 1) {
				if (itN + 1 >= road[i].Size()) continue;
				tail = road[i][itN + 1];
			}

			Vector2I dir = head - tail;
			Vector2I left = RotateWorldVec(dir);
			Vector2I right = -left;

			BuildZone zone;
			GLASSERT(size == 1 || size == 2);

			// For building, if the size is one, then the tail is at the location of the head.
			Vector2I btail = (size == 2) ? tail : head;
			zone.fullBounds.FromPair(  head + left * (porch + size),	btail + left);
			zone.buildBounds.FromPair( head + left * (porch + size),	btail + left*(1 + porch));
			zone.porchBounds.FromPair( head + left,						btail + left);

			zone.rotation = WorldRotation(right);

			Rectangle2I use = zone.fullBounds;
			use.min -= offset; use.max -= offset;
			bool okay = useMap.IsRectEmpty(use);
			if (okay && buildZones.HasCap()) {
				buildZones.Push(zone);
			}
		}
	}
	*n = buildZones.Size();
	return buildZones.Mem();
}


const RoadAI::BuildZone* RoadAI::CalcFarmZones(int* n)
{
	buildZones.Clear();
	for (int i = 0; i < MAX_ROADS; ++i) {
		if (itN >= road[i].Size())
			continue;

		Vector2I head = road[i][itN];
		Vector2I tail = road[i][itN - 1];

		// Mix up the head/tail left/right
		if (itN & 1) {
			Swap(&head, &tail);
		}

		Vector2I dir = head - tail;
		Vector2I left = RotateWorldVec(dir);
		Vector2I right = -left;

		BuildZone zone;

		Vector2I solar = head + left * (FARM_GROW_RAD+1);
		zone.porchBounds.FromPair(head + left * FARM_GROW_RAD, head + left);	// road to center.
		zone.fullBounds.min = zone.fullBounds.max = solar;
		zone.fullBounds.Outset(FARM_GROW_RAD);

		zone.buildBounds.min = zone.buildBounds.max = solar;
		zone.rotation = WorldRotation(right);

		Rectangle2I use = zone.fullBounds;
		use.min -= offset; use.max -= offset;
		bool okay = useMap.IsRectEmpty(use);
		if (okay) {
			buildZones.Push(zone);
		}
	}
	*n = buildZones.Size();
	return buildZones.Mem();
}


const Vector2I* RoadAI::GetRoad(int i, int *len)
{
	GLASSERT(i >= 0 && i < MAX_ROADS);
	*len = road[i].Size();
	return road[i].Mem();
}


const Rectangle2I* RoadAI::GetPlaza(int i)
{
	if (i < plazas.Size()) {
		return &plazas[i];
	}
	return 0;
}
