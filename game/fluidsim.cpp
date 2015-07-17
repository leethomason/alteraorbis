#include "worldmap.h"
#include "lumosmath.h"
#include "fluidsim.h"
#include "news.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/istringconst.h"

using namespace grinliz;

static const int WATERFALL_HEIGHT = 1;

static const int NDIR = 4;
static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

U8 FluidSim::floodDepth[SECTOR_SIZE_2];
grinliz::CArray<grinliz::Vector2I, SECTOR_SIZE_2> FluidSim::fillStack;
grinliz::BitArray<SECTOR_SIZE, SECTOR_SIZE, 1> FluidSim::bitFlags;


FluidSim::FluidSim(WorldMap* wm, const Vector2I& s) : worldMap(wm), settled(false)
{
	outerBounds = SectorBounds(s);
	outerBounds.DoIntersection(worldMap->Bounds());
	innerBounds = outerBounds;
	innerBounds.Outset(-1);
	type = WorldGrid::FLUID_WATER;
	nRocks = 0;
}


FluidSim::~FluidSim()
{

}


void FluidSim::EmitWaterfalls(U32 delta, Engine* engine)
{
	ParticleDef pdWater = engine->particleSystem->GetPD(ISC::fallingWater);
	ParticleDef pdLava = engine->particleSystem->GetPD(ISC::fallingLava);
	ParticleDef pdMist = engine->particleSystem->GetPD(ISC::mist);

	for (int i = 0; i<waterfalls.Size(); ++i) {
		const Vector2I& wf = waterfalls[i];
		const WorldGrid& wg = worldMap->grid[worldMap->INDEX(wf)];

		for (int j = 0; j<NDIR; ++j) {
			Vector2I v = wf + DIR[j];
			const WorldGrid& altWG = worldMap->grid[worldMap->INDEX(v)];
			int type = 0;
			if (HasWaterfall(wg, altWG, &type)) {

				Vector3F v3 = { (float)wf.x + 0.5f, (float)wg.FluidHeight(), (float)wf.y + 0.5f };
				Vector3F half = { (float)DIR[j].x*0.5f, 0.0f, (float)DIR[j].y*0.5f };
				Vector3F right = { half.z, 0, half.x };

				Rectangle3F r3;
				r3.FromPair(v3 + half - right, v3 + half + right);

				static const Vector3F DOWN = { 0, -1, 0 };
				if (type == WorldGrid::FLUID_WATER)
					engine->particleSystem->EmitPD(pdWater, r3, DOWN, delta);
				else
					engine->particleSystem->EmitPD(pdLava, r3, DOWN, delta);

				r3.min.y = r3.max.y = altWG.FluidHeight() - 0.2f;
				engine->particleSystem->EmitPD(pdMist, r3, V3F_UP, delta);
				r3.min.y = r3.max.y = 0.0f;
				engine->particleSystem->EmitPD(pdMist, r3, V3F_UP, delta);
			}
		}
	}
}



void FluidSim::Reset(int x, int y)
{
	worldMap->ResetPather(x, y);
}


bool FluidSim::HasWaterfall(const WorldGrid& wg, const WorldGrid& altWG, int* type)
{
	if (wg.IsFluid() && altWG.IsWater()) {
		if (type) {
			*type = altWG.FluidType();
		}
		return true;
	}
	return false;
}


int FluidSim::ContainsWaterfalls(const grinliz::Rectangle2I& bounds) const
{
	int count = 0;
	for (int i = 0; i < waterfalls.Size(); ++i) {
		if (bounds.Contains(waterfalls[i])) {
			++count;
		}
	}
	return count;
}


bool FluidSim::DoStep()
{
	if (settled) return true;

	memset(floodDepth, 0, SECTOR_SIZE_2*sizeof(floodDepth[0]));

	pools.Clear();
	waterfalls.Clear();
	settled = true;

	for (int d = 1; d <= MAX_ROCK_HEIGHT; ++d) {
		bitFlags.ClearAll();
		for (Rectangle2IIterator it(innerBounds); !it.Done(); it.Next()) {

			Vector2I pos2i = it.Pos();
			Vector2I loc2i = pos2i - outerBounds.min;

			const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos2i)];

			if (wg.IsWater() || wg.IsPort() || wg.IsGrid()) continue;
			if (wg.RockHeight() >= d) continue;
			if (bitFlags.IsSet(loc2i.x, loc2i.y, 0)) continue;
			// Depth 1 builds on depth 0, depth 2 on depth 1, etc.
			// But it can be rock or water.
			int depth = Max(int(floodDepth[loc2i.y*SECTOR_SIZE + loc2i.x]), int(wg.RockHeight()));
			if (depth != (d - 1)) continue;

			int waterfallStart = waterfalls.Size();
			bool bounded = FloodFill(pos2i, d, &waterfalls);
			if (bounded) {
				for (int i = 0; i < fillStack.Size(); ++i) {
					Vector2I loc = fillStack[i] - outerBounds.min;
					GLASSERT(loc.x < SECTOR_SIZE && loc.y < SECTOR_SIZE);
					floodDepth[loc.y * SECTOR_SIZE + loc.x] = d;
				}
				if (d == 1) {
					pools.Push(pos2i);
				}
			}
			else {
				while (waterfalls.Size() > waterfallStart)
					waterfalls.Pop();
			}
		}
	}
	settled = MoveFluid();
	return settled;
}


bool FluidSim::FloodFill(const Vector2I& start, int d, grinliz::CDynArray<grinliz::Vector2I>* waterfalls)
{
	fillStack.Clear();
	Vector2I locStart = start - outerBounds.min;
	GLASSERT(locStart.x < SECTOR_SIZE && locStart.y < SECTOR_SIZE);

	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(start)];

	GLASSERT(wg.IsLand() && (!wg.IsPort()) && (!wg.IsGrid()));
	GLASSERT(wg.RockHeight() < d);
	GLASSERT(floodDepth[locStart.y*SECTOR_SIZE + locStart.x] < d);
	GLASSERT(innerBounds.Contains(start));

	fillStack.Push(start);
	bitFlags.Set(locStart.x, locStart.y);

	int work = 0;
	int gridPort = 0;
	int water = 0;

	while (work < fillStack.Size()) {
		Vector2I p0 = fillStack[work];

		for (int i = 0; i < NDIR; ++i) {
			Vector2I p1 = p0 + DIR[i];
			Vector2I loc1 = p1 - outerBounds.min;
			const WorldGrid& wg1 = worldMap->grid[worldMap->INDEX(p1)];
			int index1 = loc1.y*SECTOR_SIZE + loc1.x;
			bool onEdge = (outerBounds.min.x == loc1.x) || (outerBounds.max.x == loc1.x) || (outerBounds.min.y == loc1.y) || (outerBounds.max.y == loc1.y);

			if (outerBounds.Contains(p1)
				&& !bitFlags.IsSet(loc1.x, loc1.y)
				&& wg1.RockHeight() < d)
			{
				// Whatever result, we've looked here, so don't look again.
				bitFlags.Set(loc1.x, loc1.y);

				if (wg1.IsPort() || wg1.IsGrid() || onEdge) {
					++gridPort;
				}
				else if (!wg1.IsLand()) {
					++water;
					// Push the waterfall: if this isn't 'valid'
					// the waterfall will be removed by the caller.
					if (waterfalls->Find(p0) < 0)
						waterfalls->Push(p0);
				}
				else {
					fillStack.Push(p1);
				}
			}
		}
		++work;
	}
	bool valid =    (fillStack.Size() > 3)
				 && (water < fillStack.Size() / 20)
				 && (water < 8)
				 && (gridPort == 0);
	return valid;
}


bool FluidSim::MoveFluid()
{
	bool thisSettled = true;
	nRocks = 0;

	for (Rectangle2IIterator it(outerBounds); !it.Done(); it.Next()) {
		Vector2I p = it.Pos();
		Vector2I loc2i = p - outerBounds.min;
		GLASSERT(loc2i.x < SECTOR_SIZE && loc2i.y < SECTOR_SIZE);

		WorldGrid* wg = &worldMap->grid[worldMap->INDEX(p)];
		int d = floodDepth[loc2i.y * SECTOR_SIZE + loc2i.x];

		if (wg->fluidHeight < unsigned(d * FLUID_PER_ROCK)) {
			wg->fluidHeight++;
			thisSettled = false;
		}
		else if (wg->fluidHeight > unsigned(d * FLUID_PER_ROCK)) {
			wg->fluidHeight--;
			thisSettled = false;
		}
		if (wg->RockHeight()) {
			++nRocks;
		}
	}
	return thisSettled;
}

