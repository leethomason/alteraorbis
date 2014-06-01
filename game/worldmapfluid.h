#ifndef WORLDMAP_FLUID_SIM_INCLUDED
#define WORLDMAP_FLUID_SIM_INCLUDED

#include "../grinliz/glcontainer.h"

class WorldMap;

class FluidSim
{
public:
	FluidSim(WorldMap* worldMap);
	~FluidSim();

	void DoStep(const grinliz::Vector2I& bounds);

private:
	void Reset(int x, int y);
	void SetFlag(int x, int y)	{ flag[y*SECTOR_SIZE + x] = true; }
	bool Flag(int x, int y) { return flag[y*SECTOR_SIZE + x]; }
		
	WorldMap* worldMap;
	int iteration;
	grinliz::CDynArray<grinliz::Vector2I> emitters;
	grinliz::CDynArray<grinliz::Vector2I> stack;

	S8 water[SECTOR_SIZE*SECTOR_SIZE];
	bool flag[SECTOR_SIZE*SECTOR_SIZE];
};

#endif // WORLDMAP_FLUID_SIM_INCLUDED
