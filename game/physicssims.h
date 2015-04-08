#ifndef PHYSICS_SIMS_INCLUDED
#define PHYSICS_SIMS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../xegame/cticker.h"
#include "gamelimits.h"

class ChitContext;
class XStream;
class FluidSim;
class CircuitSim;

class PhysicsSims
{
public:
	PhysicsSims(const ChitContext* context);
	~PhysicsSims();

	void DoTick(int delta);
	void Serialize( XStream* xs );

	FluidSim* GetFluidSim(const grinliz::Vector2I& sector) const {
		GLASSERT(sector.x >= 0 && sector.x < NUM_SECTORS);
		GLASSERT(sector.y >= 0 && sector.y < NUM_SECTORS);
		return fluidSim[sector.y*NUM_SECTORS + sector.x];
	}

	CircuitSim* GetCircuitSim(const grinliz::Vector2I& sector) const {
		GLASSERT(sector.x >= 0 && sector.x < NUM_SECTORS);
		GLASSERT(sector.y >= 0 && sector.y < NUM_SECTORS);
		return circuitSim[sector.y*NUM_SECTORS + sector.x];
	}
private:
	const ChitContext* context;
	CTicker		fluidTicker;
	int			fluidSector;
	CircuitSim*	circuitSim[NUM_SECTORS*NUM_SECTORS];
	FluidSim*	fluidSim[NUM_SECTORS*NUM_SECTORS];
};


#endif // PHYSICS_SIMS_INCLUDED
