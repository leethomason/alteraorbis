#ifndef WORLDMAP_FLUID_SIM_INCLUDED
#define WORLDMAP_FLUID_SIM_INCLUDED

#include "../grinliz/glcontainer.h"

class WorldMap;

class FluidSim
{
public:
	FluidSim(WorldMap* worldMap, const grinliz::Rectangle2I& bounds);
	~FluidSim();

	// Call every 600ms (?)
	bool DoStep();
	// Particle calls
	void EmitWaterfalls(U32 delta, Engine* engine);

	bool Settled() const { return settled; }
	void Unsettle() { settled = false; }

private:
	void Reset(int x, int y);
	U32 Hash();
	bool HasWaterfall(const WorldGrid& origin, const WorldGrid& adjacent);
		
	WorldMap* worldMap;
	grinliz::Rectangle2I bounds;
	bool settled;

	grinliz::CDynArray<grinliz::Vector2I> waterfalls;

	// Local, cached: (don't want to re-allocate)
	grinliz::CDynArray<grinliz::Vector2I> emitters;
	grinliz::CDynArray<int> emitterHeights;
	grinliz::CDynArray<grinliz::Vector2I> stack;

	grinliz::BitArray<SECTOR_SIZE, SECTOR_SIZE, 1> flag;
	S8 water[SECTOR_SIZE*SECTOR_SIZE];
};

#endif // WORLDMAP_FLUID_SIM_INCLUDED
