#ifndef WORLDMAP_FLUID_SIM_INCLUDED
#define WORLDMAP_FLUID_SIM_INCLUDED

#include "../grinliz/glcontainer.h"
#include "../grinliz/glbitarray.h"

class WorldMap;
struct WorldGrid;

/*
	So many many iterations of this code.
	Things to think about:
	- Waterfalls are cool, and water at different heights is cool.
		Put a lot of work into this, and still doesn't work. But
		could; detect closed regions, flow from one closed region
		to another.
	- It's a flat world. can't do a real sim
	- Water "piles" are disturbing. Water really needs to be bounded.
		This was the big problem in the previous implementation.
		If the water piles are bothersome, then a "mostly closed"
		algorithm works fine. But see the pather stuff.
	- Lava piles are pretty cool.
	- The pather...is a complex topic. 
		If some things can go through fluids and some things can't,
		then there needs to be different solutions to pathing.
		That can be implemented: currentPathState or something.
		But adds complexity.
*/
class FluidSim
{
public:
	FluidSim(WorldMap* worldMap, const grinliz::Vector2I& sector);
	~FluidSim();

	// Call every 600ms (?)
	bool DoStep();
	// Particle calls
	void EmitWaterfalls(U32 delta, Engine* engine);

	bool Settled() const { return settled; }
	void Unsettle() { settled = false; }

	int NumWaterfalls() const { return waterfalls.Size(); }
	int NumPools() const { return pools.Size(); }

	grinliz::Vector2I PoolLoc(int i) const { return pools[i]; }

	int ContainsWaterfalls(const grinliz::Rectangle2I& bounds) const;
	
	//int FindEmitter(const grinliz::Vector2I& start, bool nominal, bool magma, int* area);
	grinliz::Rectangle2I Bounds() const { return outerBounds; }

private:
	void Reset(int x, int y);
	bool HasWaterfall(const WorldGrid& origin, const WorldGrid& adjacent, int* type);
	void PressureStep();

	bool FloodFill(const grinliz::Vector2I& start, int h,
				   grinliz::CDynArray<grinliz::Vector2I>* emitters, 
				   grinliz::CDynArray<grinliz::Vector2I>* waterfalls);
	bool MoveFluid();

	// Both bounds check set 'flag' to tell where the pressureized parts should be.
	// if 'waterfalls' is set, it only needs to be mostly enclosed at h >= 2
	int BoundCheck(const grinliz::Vector2I& start, int h, bool nominal, bool magma);

	grinliz::Vector2I MaxAdjacentWater(int i, int j);

	WorldMap* worldMap;
	grinliz::Rectangle2I outerBounds, innerBounds;
	bool settled;

	grinliz::CDynArray<grinliz::Vector2I> waterfalls;

	grinliz::CDynArray<grinliz::Vector2I> emitters, pools;

	// can be shared - scratch memory.
	static U8 floodDepth[SECTOR_SIZE_2];
	static grinliz::CArray<grinliz::Vector2I, SECTOR_SIZE_2> fillStack;
	static grinliz::BitArray<SECTOR_SIZE, SECTOR_SIZE, 1> bitFlags;
};

#endif // WORLDMAP_FLUID_SIM_INCLUDED
