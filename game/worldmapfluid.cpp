#include "worldmap.h"
#include "lumosmath.h"
#include "worldmapfluid.h"
#include "../engine/engine.h"
#include "../engine/particle.h"

using namespace grinliz;

static const int PRESSURE = 25;
static const int WATERFALL_HEIGHT = 3;

static const int NDIR = 4;
static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

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
	worldMap->voxelInit.Clear(x / WorldMap::ZONE_SIZE, y / WorldMap::ZONE_SIZE);
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

bool FluidSim::DoStep()
{
	if (settled) return true;

	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	Rectangle2I aoe;
	aoe.SetInvalid();
	
	emitters.Clear();
	emitterHeights.Clear();
	U32 startHash = Hash();

	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {

			Vector2I pos2i = { i, j };
			int index = worldMap->INDEX(i, j);
			WorldGrid* worldGrid = &worldMap->grid[index];

			if (worldGrid->FluidSink()) {
				water[j*SECTOR_SIZE + i] = 0;
			}
			else if (worldGrid->fluidEmitter) {
				// Find highest rock neighbor.
				int h = FLUID_PER_ROCK;
				for (int k = 0; k < NDIR; ++k) {
					h = Max(h, (int)worldMap->grid[worldMap->INDEX(i + DIR[k].x, j + DIR[k].y)].rockHeight * FLUID_PER_ROCK);
				}

				water[j*SECTOR_SIZE + i] = h;
				emitters.Push(pos2i);
				emitterHeights.Push(h);
			}
			else {
				int max = 0;

				for (int k = 0; k < NDIR; ++k) {
					int subIndex = worldMap->INDEX(i + DIR[k].x, j + DIR[k].y);
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
#if 0
				for (int k = 0; k < NDIR; ++k) {
					// Check that we have at least one path to the diagonal.
					// Somewhat obnoxious code...but looks funny water going
					// through cracks.
					
					Vector2I dir0 = DIR[k];
					Vector2I dir1 = DIR[(k + 1) % NDIR];
					GLASSERT(DotProduct(dir0, dir1) == 0);
					int subIndex0 = worldMap->INDEX(i + dir0.x, j + dir0.y);
					int subIndex1 = worldMap->INDEX(i + dir1.x, j + dir1.y);
					const WorldGrid* subGrid0 = &worldMap->grid[subIndex0];
					const WorldGrid* subGrid1 = &worldMap->grid[subIndex1];

					if (subGrid0->IsFluid() || subGrid1->IsFluid()) {
						Vector2I dir = dir0 + dir1;
						int subIndex = worldMap->INDEX(i + dir.x, j + dir.y);
						const WorldGrid* subGrid = &worldMap->grid[subIndex];
						int h = subGrid->RockHeight() ? 1 + (int)subGrid->fluidHeight - (int)subGrid->RockHeight()*FLUID_PER_ROCK : (int)subGrid->fluidHeight;
						max = Max(max, h);
					}

				}
#endif
				int wgBottom = worldGrid->RockHeight() * FLUID_PER_ROCK;

				if (worldGrid->fluidHeight > 0 && (int)worldGrid->fluidHeight >= max)  {
					// this is the higher fluid...flow down.
					water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight - 1;
				}
				else if (    (int)worldGrid->fluidHeight < max - 1	// lower fluid
					      && wgBottom < max - 1  )					// and we have space over the rock
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

	for (int j = 1; j < SECTOR_SIZE - 1; ++j) {
		for (int i = 1; i < SECTOR_SIZE - 1; ++i) {
			Vector2I pos = { bounds.min.x + i, bounds.min.y + j };
			int index = worldMap->INDEX(pos);
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
	// Pressure from the emitter.
	for (int i = 0; i < emitters.Size(); ++i) {
		Vector2I start = emitters[i];
		int h = emitterHeights[i];
		flag.ClearAll();

		stack.Clear();
		stack.Push(start);
		flag.Set(start.x, start.y);
		int area = 0;

		while (!stack.Empty() && area < PRESSURE) {
			Vector2I p = stack.PopFront();	// Need to PopFront() to get 'round' areas and not go depth-first
			++area;

			int index = worldMap->INDEX(p);

			if (worldMap->grid[index].FluidSink()) {
				// done!
				area = PRESSURE;
				continue;
			}

			// Note that water, above, goes over
			// rock. But pressure does not.
			if (worldMap->grid[index].RockHeight())
				continue;

			if (worldMap->grid[index].fluidHeight > 1) {
				if ((int)worldMap->grid[index].fluidHeight < h) {
					worldMap->grid[index].fluidHeight++;
				}
			}

			for (int k = 0; k < NDIR; ++k) {
				Vector2I q = p + DIR[k];
				int qIndex = worldMap->INDEX(q);
				if (worldMap->grid[qIndex].fluidHeight > 1) {
					if (!flag.IsSet(q.x, q.y)) {
						stack.Push(q);
						flag.Set(q.x, q.y);
					}
				}
			}
		}
	}
#endif
	U32 endHash = Hash();
	
	settled = startHash == endHash;
	if (!settled) {
		waterfalls.Clear();
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
	}
	return settled;
}
