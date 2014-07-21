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
	bool DoStep(grinliz::Rectangle2I* aoe);
	// Particle calls
	void EmitWaterfalls(U32 delta, Engine* engine);

	bool Settled() const { return settled; }
	void Unsettle() { settled = false; }

	int NumWaterfalls() const { return waterfalls.Size(); }
	int NumPools() const { return emitters.Size(); }
	grinliz::Vector2I PoolLoc(int i) const { return emitters[i]; }

	int ContainsWaterfalls(const grinliz::Rectangle2I& bounds) const;
	
	int FindEmitter(const grinliz::Vector2I& start, bool nominal, bool magma);

private:
	void Reset(int x, int y);
	U32 Hash();
	bool HasWaterfall(const WorldGrid& origin, const WorldGrid& adjacent, int* type);
	void PressureStep();

	// Both bounds check set 'flag' to tell where the pressureized parts should be.
	// Do a bounds check for magma flow - unlike water, just needs to be "mostly" enclosed.
	int BoundCheck(const grinliz::Vector2I& start, int h, bool nominal, bool magma);

	grinliz::Vector2I MaxAdjacentWater(int i, int j);

	enum { PRESSURE = 25 };

	WorldMap* worldMap;
	grinliz::Rectangle2I bounds;
	bool settled;

	grinliz::CDynArray<grinliz::Vector2I> waterfalls;

	// Local, cached: (don't want to re-allocate)
	grinliz::CDynArray<grinliz::Vector2I> emitters;
	grinliz::CDynArray<grinliz::Vector2I> stack;

	// can be shared - scratch memory.
	static S8 water[SECTOR_SIZE*SECTOR_SIZE];
	static U8 boundHeight[SECTOR_SIZE*SECTOR_SIZE];
	static grinliz::CArray<grinliz::Vector2I, PRESSURE> fill;

	grinliz::BitArray<SECTOR_SIZE, SECTOR_SIZE, 1> flag;
};

#endif // WORLDMAP_FLUID_SIM_INCLUDED
