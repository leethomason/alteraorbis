#include "worldmap.h"
#include "lumosmath.h"
#include "worldmapfluid.h"

using namespace grinliz;

static const int PRESSURE = 25;

FluidSim::FluidSim(WorldMap* wm) : worldMap(wm)
{
}


FluidSim::~FluidSim()
{

}


void FluidSim::Reset(int x, int y)
{
	worldMap->voxelInit.Clear(x / WorldMap::ZONE_SIZE, y / WorldMap::ZONE_SIZE);
	worldMap->ResetPather(x, y);
}

void FluidSim::DoStep(const grinliz::Vector2I& sector)
{
	Rectangle2I bounds = SectorBounds(sector);
	Vector2I center = bounds.Center();
	static const int NDIR = 4;
	static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	
	emitters.Clear();

	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {

			Vector2I pos2i = { i, j };
			int index = worldMap->INDEX(i, j);
			WorldGrid* worldGrid = &worldMap->grid[index];

			if (worldGrid->FluidSink()) {
				water[j*SECTOR_SIZE + i] = 0;
			}
			else if (worldGrid->fluidEmitter) {
				water[j*SECTOR_SIZE + i] = 4;
				if (emitters.Find(pos2i) < 0) {
					emitters.Push(pos2i);
				}
			}
			else {
				int max = 0;
				for (int k = 0; k < NDIR; ++k) {
					int subIndex = worldMap->INDEX(i + DIR[k].x, j + DIR[k].y);
					const WorldGrid* subGrid = &worldMap->grid[subIndex];
					if (subGrid->IsFluid()) {
						max = Max(max, (int)subGrid->fluidHeight - (int)subGrid->RockHeight()*FLUID_PER_ROCK);
					}
				}
#if 1
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
						max = Max(max, (int)subGrid->fluidHeight - (int)subGrid->RockHeight()*FLUID_PER_ROCK);
					}

				}
#endif
				if (worldGrid->fluidHeight && (int)worldGrid->fluidHeight >= max)  {
					water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight - 1;
				}
				else if ((int)worldGrid->fluidHeight < max - 1) {
					water[j*SECTOR_SIZE + i] = worldGrid->fluidHeight + 1;
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
			worldMap->grid[index].fluidHeight = water[j*SECTOR_SIZE + i];
		}
	}

#if 1
	// Pressure from the emitter.
	for (int i = 0; i < emitters.Size(); ++i) {
		Vector2I start = emitters[i];
		flag.ClearAll();

		stack.Clear();
		stack.Push(start);
		flag.Set(start.x, start.y);
		int area = 0;

		while (!stack.Empty() && area < PRESSURE) {
			Vector2I p = stack.PopFront();	// Need to PopFront() to get 'round' areas and not go depth-first
			++area;

			int index = worldMap->INDEX(p);

			// Note that water, above, goes over
			// rock. But pressure does not.
			if (worldMap->grid[index].RockHeight())
				continue;

			if (worldMap->grid[index].fluidHeight > 1) {
				if (worldMap->grid[index].fluidHeight < 4)
					worldMap->grid[index].fluidHeight++;
			}
			worldMap->grid[index].fluidHeight = 4;

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
}
