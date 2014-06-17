#include "worldmap.h"
#include "lumosmath.h"
#include "worldmapfluid.h"
#include "news.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

using namespace grinliz;

static const int PRESSURE = 25;
static const int WATERFALL_HEIGHT = 3;

static const int NDIR = 4;
static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

S8 FluidSim::water[SECTOR_SIZE*SECTOR_SIZE];

FluidSim::FluidSim(WorldMap* wm, const Rectangle2I& b) : worldMap(wm), bounds(b)
{
}


FluidSim::~FluidSim()
{

}


void FluidSim::EmitWaterfalls(U32 delta, Engine* engine)
{
	ParticleDef pdWater = engine->particleSystem->GetPD("fallingwater");
	ParticleDef pdMist = engine->particleSystem->GetPD("mist");

	for (int i = 0; i<waterfalls.Size(); ++i) {
		const Vector2I& wf = waterfalls[i];
		const WorldGrid& wg = worldMap->grid[worldMap->INDEX(wf)];

		for (int j = 0; j<NDIR; ++j) {
			Vector2I v = wf + DIR[j];
			const WorldGrid& altWG = worldMap->grid[worldMap->INDEX(v)];
			if (HasWaterfall(wg, altWG)) {

				Vector3F v3 = { (float)wf.x + 0.5f, (float)altWG.FluidHeight(), (float)wf.y + 0.5f };
				Vector3F half = { (float)DIR[j].x*0.5f, 0.0f, (float)DIR[j].y*0.5f };
				Vector3F right = { half.z, 0, half.x };

				Rectangle3F r3;
				r3.FromPair(v3 + half*0.8f - right, v3 + half*0.8f + right);

				static const Vector3F DOWN = { 0, -1, 0 };
				engine->particleSystem->EmitPD(pdWater, r3, DOWN, delta);

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
//	worldMap->voxelInit.Clear(x / WorldMap::ZONE_SIZE, y / WorldMap::ZONE_SIZE);
	worldMap->ResetPather(x, y);
}


U32 FluidSim::Hash()
{
	unsigned int h = 2166136261U;

	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {
			const WorldGrid& wg = worldMap->grid[worldMap->INDEX(i, j)];
			int value = wg.fluidHeight + 32 * wg.fluidEmitter;
			h ^= value;
			h *= 16777619;
		}
	}
	return h;
}


bool FluidSim::HasWaterfall(const WorldGrid& wg, const WorldGrid& altWG)
{
	if (   (altWG.IsFluid() && wg.IsFluid() && altWG.fluidHeight >= wg.fluidHeight + WATERFALL_HEIGHT)
		|| (wg.IsWater() && altWG.fluidHeight >= WATERFALL_HEIGHT))
	{
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


bool FluidSim::DoStep(Rectangle2I* _aoe)
{
	if (settled) return true;

	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	Rectangle2I aoe;
	aoe.SetInvalid();

	emitters.Clear();
	emitterHeights.Clear();
	waterfalls.Clear();
	U32 startHash = Hash();

	for (int y = bounds.min.y + 1; y < bounds.max.y; ++y) {
		for (int x = bounds.min.x + 1; x < bounds.max.x; ++x) {

			int i = x - bounds.min.x;
			int j = y - bounds.min.y;

			Vector2I pos2i = { x, y };
			int index = worldMap->INDEX(pos2i);
			WorldGrid* worldGrid = &worldMap->grid[index];

			if (worldGrid->FluidSink()) {
				water[j*SECTOR_SIZE + i] = 0;
			}
			else if (worldGrid->fluidEmitter) {
				// Are we emitting? If not, should we be?
				// Find highest rock neighbor.
				/*if (worldGrid->IsFluid()) 
				{
					GLASSERT(worldGrid->fluidHeight % FLUID_PER_ROCK == 0);	// nothing should change emitter.
					// just keep emitting. Once the emitter 'fires' it doesn't stop
					// until rock is built on it.
					// FIXME: infinite emitters. Volcanos should clear emitters.
					int h = worldGrid->fluidHeight;
					water[j*SECTOR_SIZE + i] = h;
					emitters.Push(pos2i);
					emitterHeights.Push(h);
				}
				else */
				{
					int hRock = FindEmitter(pos2i, false);
					if (hRock) {
						int h = FLUID_PER_ROCK * hRock;
						water[j*SECTOR_SIZE + i] = h;
						emitters.Push(pos2i);
						emitterHeights.Push(h);
					}
					else if (worldGrid->IsFluid()) {
						water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight - 1;
					}
				}
			}
			else {
				int max = 0;

				for (int k = 0; k < NDIR; ++k) {
					int subIndex = worldMap->INDEX(x + DIR[k].x, y + DIR[k].y);
					const WorldGrid* subGrid = &worldMap->grid[subIndex];
					if ((int)subGrid->fluidHeight > max) {
						max = Max(max, (int)subGrid->fluidHeight);

						// Water falls as it goes over rock - adjust the
						// effective height here.
						if (subGrid->RockHeight() > worldGrid->RockHeight()) {
							max -= (subGrid->RockHeight() - worldGrid->RockHeight()) * FLUID_PER_ROCK - 1;
						}
					}
				}

				int wgBottom = worldGrid->RockHeight() * FLUID_PER_ROCK;

				if (worldGrid->fluidHeight > 0 && (int)worldGrid->fluidHeight >= max)  {
					// this is the higher fluid...flow down.
					water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight - 1;
				}
				else if ((int)worldGrid->fluidHeight < max - 1	// lower fluid
					&& wgBottom < max - 1)					// and we have space over the rock
				{
					// Go up, but be sure to clear the rock.
					water[j*SECTOR_SIZE + i] = Max((int)worldGrid->fluidHeight + 1, wgBottom + 1);
				}
				else {
					water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight;
				}
			}
		}
	}

	for (int y = bounds.min.y + 1; y < bounds.max.y; ++y) {
		for (int x = bounds.min.x + 1; x < bounds.max.x; ++x) {
			Vector2I pos = { x, y };
			int index = worldMap->INDEX(pos);

			int i = x - bounds.min.x;
			int j = y - bounds.min.y;

			int h = water[j*SECTOR_SIZE + i];
			if (h) {
				aoe.DoUnion(pos);
			}
			if (worldMap->grid[index].fluidHeight != h) {
				worldMap->grid[index].fluidHeight = h;
				GLASSERT(worldMap->grid[index].fluidHeight == water[j*SECTOR_SIZE + i]);	// check for bit field errors
			}
		}
	}

#if 1
	for (int i = 0; i < emitters.Size(); ++i) {
		PressureStep(emitters[i], emitterHeights[i]);
	}
#endif

	// Reset the pather.
	// Scan for waterfalls.
	aoe.Outset(1);
	aoe.DoIntersection(bounds);

	for (Rectangle2IIterator it(aoe); !it.Done(); it.Next()) {
		Reset(it.Pos().x, it.Pos().y);

		const WorldGrid& wg = worldMap->grid[worldMap->INDEX(it.Pos())];
		for (int k = 0; k < NDIR; ++k) {
			const WorldGrid& altWG = worldMap->grid[worldMap->INDEX(it.Pos() + DIR[k])];
			if (HasWaterfall(wg, altWG)) {
				waterfalls.Push(it.Pos());
				// Only need the initial location - not all the waterfalls.
				break;
			}
		}
	}

	U32 endHash = Hash();
	settled = (startHash == endHash);
	if (_aoe) *_aoe = aoe;
	return settled;
}


int FluidSim::PressureStep(const Vector2I& start, int h)
{
	// Pressure from the emitter.
	flag.ClearAll();

	stack.Clear();
	stack.Push(start);
	flag.Set(start.x - bounds.min.x, start.y - bounds.min.y);
	int area = 0;
	int startRockHeight = worldMap->grid[worldMap->INDEX(start)].RockHeight();

	while (!stack.Empty() && area < PRESSURE) {
		Vector2I p = stack.PopFront();	// Need to PopFront() to get 'round' areas and not go depth-first

		int index = worldMap->INDEX(p);
		++area;

		if (worldMap->grid[index].FluidSink()) {
			break;
		}

		if ((p!=start) && worldMap->grid[index].fluidHeight > 1) {
			if ((int)worldMap->grid[index].fluidHeight < h) {
				worldMap->grid[index].fluidHeight++;
			}
		}

		for (int k = 0; k < NDIR; ++k) {
			Vector2I q = p + DIR[k];
			Vector2I qLoc = { q.x - bounds.min.x, q.y - bounds.min.y };

			int qIndex = worldMap->INDEX(q);
			const WorldGrid& qWG = worldMap->grid[qIndex];

			if (!flag.IsSet(qLoc.x, qLoc.y)) {
				// Pressure doesn't go through ground changes. (this
				// could be modelled in more detail.)
				if (qWG.IsFluid() && qWG.RockHeight() == startRockHeight) {
					stack.Push(q);
					flag.Set(qLoc.x, qLoc.y);
				}
			}
		}
	}
	return area;
}


int FluidSim::FindEmitter(const grinliz::Vector2I& pos2i, bool nominal)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos2i)];

	for (int hRock = MAX_ROCK_HEIGHT; hRock > 0; hRock--) {
		if (hRock > wg.RockHeight()) {
			int h = hRock * FLUID_PER_ROCK;
			int area = 0, areaElevated = 0;
			BoundCheck(pos2i, h / FLUID_PER_ROCK, nominal, &area, &areaElevated);
			// Don't start an emitter unless it is enclosed.
			// "water patties" look pretty silly.
			if (area > 5 && area < PRESSURE) {
				return hRock;
			}
		}
	}
	return 0;
}


void FluidSim::BoundCheck(const Vector2I& start, int h, int nominal, int* area, int* areaElevated )
{
	// Pressure from the emitter.
	flag.ClearAll();

	stack.Clear();
	stack.Push(start);
	flag.Set(start.x - bounds.min.x, start.y - bounds.min.y);

	while (!stack.Empty() && *area < PRESSURE) {
		Vector2I p = stack.PopFront();	// Need to PopFront() to get 'round' areas and not go depth-first

		int index = worldMap->INDEX(p);
		const WorldGrid& wg = worldMap->grid[index];

		*area += 1;

		int wgRockHeight = nominal ? wg.NominalRockHeight() : wg.RockHeight();
		if (wgRockHeight) {
			*areaElevated += 1;
		}
		if (wg.FluidSink()) {
			continue;
		}

		int push = 0;
		int pass = 0;

		for (int k = 0; k < NDIR; ++k) {
			Vector2I q = p + DIR[k];
			Vector2I qLoc = { q.x - bounds.min.x, q.y - bounds.min.y };

			int qIndex = worldMap->INDEX(q);
			int qRockHeight = nominal ? worldMap->grid[qIndex].NominalRockHeight() : worldMap->grid[qIndex].RockHeight();

			if ( qRockHeight < h) {
				++pass;
				if (!flag.IsSet(qLoc.x, qLoc.y)) {
					stack.Push(q);
					++push;
					flag.Set(qLoc.x, qLoc.y);
				}
			}
		}
		// This is a bit of craziness. Narrow passages are
		// considered bounding. (And create waterfalls.)
		// Detect a narrow passage if there are 2 ways
		// out and one way is marked already. Then
		// undo to bound the fill.
		if (push == 1 && pass == 2) {
			stack.Pop();
		}
	}
}
