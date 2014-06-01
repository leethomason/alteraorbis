#include "worldmap.h"
#include "lumosmath.h"
#include "worldmapfluid.h"

using namespace grinliz;

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
	memset(flag, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(flag[0]));

#if 1
	// convolution
	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {

			int index = worldMap->INDEX(i, j);

			// FIXME: clean up rules.
			if (worldMap->grid[index].RockHeight())
				continue;	// or grid or other stuff??
			if (worldMap->grid[index].waterHeight) {
				int debug = 1;
			}
			
			static const int grid[3][3] = {
				/*
				{ 0, 1, 0 },
				{ 1, 1, 1 },
				{ 0, 1, 0 }
				*/
				{ 0, 4, 0 },
				{ 4, 1, 4 },
				{ 0, 4, 0 }
			};

			int total = 0;
			int gridTotal = 0;
			for (int dy = -1; dy <= 1; ++dy) {
				for (int dx = -1; dx <= 1; ++dx) {
					int subIndex = worldMap->INDEX(i + dx, j + dy);
					WorldGrid* subGrid = &worldMap->grid[subIndex];
					if (subGrid->RockHeight() == 0) {
						if (subGrid->waterHeight || (dx == 0 && dy == 0)) {
							int cell = grid[1 + dy][1 + dx];
							total += subGrid->waterHeight * cell;
							gridTotal += cell;
						}
					}
				}
			}
			water[j*SECTOR_SIZE + i] = gridTotal ? (total + (gridTotal+0)/2) / gridTotal : 0;
		}
	}

	for (int j = 1; j < SECTOR_SIZE - 1; ++j) {
		for (int i = 1; i < SECTOR_SIZE - 1; ++i) {
			Vector2I pos = { bounds.min.x + i, bounds.min.y + j };

			int index = worldMap->INDEX(pos);
			if (worldMap->grid[index].fluidEmitter) {
				worldMap->grid[index].waterHeight = 4;
				if (emitters.Find(pos) < 0) {
					emitters.Push(pos);
				}
			}
			else {
				worldMap->grid[index].waterHeight = water[j*SECTOR_SIZE + i];
			}
		}
	}
#endif

#if 0
	// Pressure from the emitter.
	for (int i = 0; i < emitters.Size(); ++i) {
		Vector2I start = emitters[i];
		stack.Clear();
		stack.Push(start);
		SetFlag(start.x, start.y);
		int area = 0;

		while (!stack.Empty() && area < 20) {
			Vector2I p = stack.Pop();
			SetFlag(p.x, p.y);
			++area;

			int index = worldMap->INDEX(p);
			if (worldMap->grid[index].waterHeight > 1) {
				if (worldMap->grid[index].waterHeight < 4)
					worldMap->grid[index].waterHeight++;
			}

			for (int k = 0; k < NDIR; ++k) {
				Vector2I q = p + DIR[k];
				int i2 = worldMap->INDEX(q);
				if (worldMap->grid[i2].waterHeight > 1) {
					if (!Flag(q.x, q.y)) {
						stack.Push(q);
					}
				}
			}
		}
	}
#endif
}
