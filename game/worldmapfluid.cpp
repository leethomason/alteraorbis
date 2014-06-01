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
	//static const int NDIR = 2;
	//static const Vector2I DIR[NDIR] = { { -1, 0 }, { 0, -1 } };
	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	memset(flag, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(flag[0]));
#if 0
//	int y0 = (iteration & 1) ? 1 : 0;
//	int x0 = (iteration & 2) ? 1 : 0;

	for (int j = bounds.min.y + 1; j < bounds.max.y; j++) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; i++) {
		//for (int i = bounds.max.x - 1; i > bounds.min.x; --i) {

			int index = worldMap->INDEX(i, j);
			WorldGrid* wg = &worldMap->grid[index];

			if (wg->RockHeight() || !wg->IsLand())
				continue;	// or grid or other stuff??
			if (wg->fluidEmitter)
				wg->waterHeight = 4;

			if (wg->waterHeight == 0 )
				continue;

			if (wg->waterHeight) {
				int debug = 1;
			}

			for (int k = 0; k < NDIR; ++k) {
				Vector2I dir = DIR[k];
				int subIndex = worldMap->INDEX(i + dir.x, j + dir.y);
				WorldGrid* subGrid = &worldMap->grid[subIndex];

				if (subGrid->RockHeight() == 0 ) {
					if (subGrid->waterHeight < wg->waterHeight) {
						subGrid->waterHeight += 1;
						break;
					}
				}
				/*
				if (subGrid->RockHeight() == 0) {
					if (subGrid->waterHeight < wg->waterHeight - 1) {
						subGrid->waterHeight += 1;
						wg->waterHeight -= 1;
						break;
					}
				}*/
			}
		}
	}
#endif
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
			/*
			static const int grid[3][3] = {
				{ 0, 1, 0 },
				{ 1, 2, 1 },
				{ 0, 1, 0 }
			};
			*/
			
			static const int grid[3][3] = {
				{ 0, 4, 0 },
				{ 4, 5, 4 },
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
			water[j*SECTOR_SIZE + i] = gridTotal ? (total + gridTotal/2) / gridTotal : 0;
		}
	}

	for (int j = 1; j < SECTOR_SIZE - 1; ++j) {
		for (int i = 1; i < SECTOR_SIZE - 1; ++i) {

			int index = worldMap->INDEX(bounds.min.x+i, bounds.min.y+j);
			if (worldMap->grid[index].fluidEmitter)
				worldMap->grid[index].waterHeight = 4;
			else
				worldMap->grid[index].waterHeight = water[j*SECTOR_SIZE + i];
		}
	}
#endif
}
