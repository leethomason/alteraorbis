#include "worldmap.h"
#include "lumosmath.h"
#include "worldmapfluid.h"

using namespace grinliz;

static const int PRESSURE = 25;

FluidSim::FluidSim(WorldMap* wm) : worldMap(wm)
{
	iteration = 0;
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
	iteration++;
	Rectangle2I bounds = SectorBounds(sector);
	Vector2I center = bounds.Center();
	static const int NDIR = 4;
	static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));

	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {

			Vector2I pos2i = { i, j };
			int index = worldMap->INDEX(i, j);
			WorldGrid* worldGrid = &worldMap->grid[index];

			// FIXME: clean up rules.
			if (worldGrid->RockHeight())
				continue;	// or grid or other stuff??
			if (worldGrid->waterHeight) {
				int debug = 1;
			}
			
			if (worldGrid->fluidEmitter) {
				water[j*SECTOR_SIZE + i] = 4;
				if (emitters.Find(pos2i) < 0) {
					emitters.Push(pos2i);
				}
			}
			else {
				int max = 0;
				for (int k = 0; k < 4; ++k) {
					int subIndex = worldMap->INDEX(i + DIR[k].x, j + DIR[k].y);
					WorldGrid* subGrid = &worldMap->grid[subIndex];
					max = Max(max, (int)subGrid->waterHeight);
				}
				for (int k = 0; k < 4; ++k) {
					Vector2I dir = DIR[k] + DIR[(k + 1) % NDIR];
					int subIndex = worldMap->INDEX(i + dir.x, j + dir.y);
					WorldGrid* subGrid = &worldMap->grid[subIndex];
					max = Max(max, (int)subGrid->waterHeight);
				}
				if (worldGrid->waterHeight && (int)worldGrid->waterHeight >= max)  {
					water[j*SECTOR_SIZE + i] = worldGrid->waterHeight - 1;
				}
				else if ((int)worldGrid->waterHeight < max - 1) {
					water[j*SECTOR_SIZE + i] = worldGrid->waterHeight + 1;
				}
				else {
					water[j*SECTOR_SIZE + i] = worldGrid->waterHeight;
				}
			}
		}
	}

	for (int j = 1; j < SECTOR_SIZE - 1; ++j) {
		for (int i = 1; i < SECTOR_SIZE - 1; ++i) {
			Vector2I pos = { bounds.min.x + i, bounds.min.y + j };
			int index = worldMap->INDEX(pos);
			worldMap->grid[index].waterHeight = water[j*SECTOR_SIZE + i];
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
			Vector2I p = stack.PopFront();
			++area;

			int index = worldMap->INDEX(p);
			if (worldMap->grid[index].RockHeight())
				continue;

			if (worldMap->grid[index].waterHeight > 1) {
				if (worldMap->grid[index].waterHeight < 4)
					worldMap->grid[index].waterHeight++;
			}
			worldMap->grid[index].waterHeight = 4;

			for (int k = 0; k < NDIR; ++k) {
				Vector2I q = p + DIR[k];
				int qIndex = worldMap->INDEX(q);
				if (worldMap->grid[qIndex].waterHeight > 1) {
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
