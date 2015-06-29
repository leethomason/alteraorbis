#include "physicssims.h"
#include "../xegame/chitcontext.h"
#include "../Shiny/include/Shiny.h"
#include "circuitsim.h"
#include "fluidsim.h"
#include "lumosmath.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

PhysicsSims::PhysicsSims(const ChitContext* _context) : context(_context)
{
	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		circuitSim[i] = 0;
		fluidSim[i] = 0;
	}
	GLASSERT(context->worldMap);
	{
		Vector2I sector = { 0, 0 };
		// 0,0 used by the test scenes. The other border sims not needed.
		circuitSim[0] = new CircuitSim(context, sector);
		fluidSim[0] = new FluidSim(context->worldMap, sector);
	}
	for (int j = 1; j < NUM_SECTORS - 1; ++j) {
		for (int i = 1; i < NUM_SECTORS - 1; ++i) {
			Vector2I sector = { i, j };
			circuitSim[j*NUM_SECTORS + i] = new CircuitSim(context, sector);
			fluidSim[j*NUM_SECTORS + i] = new FluidSim(context->worldMap, sector);
		}
	}
	fluidTicker.SetPeriod(600 / (NUM_SECTORS*NUM_SECTORS));
	fluidSector = 0;
}

PhysicsSims::~PhysicsSims()
{
	for (int j = 0; j < NUM_SECTORS; ++j) {
		for (int i = 0; i < NUM_SECTORS; ++i) {
			delete circuitSim[j*NUM_SECTORS + i];
			delete fluidSim[j*NUM_SECTORS + i];
		}
	}
}


void PhysicsSims::Serialize(XStream* xs)
{
	XarcOpen(xs, "PhysicsSim");
	for (int j = 0; j < NUM_SECTORS; ++j) {
		for (int i = 0; i < NUM_SECTORS; ++i) {
			XarcOpen(xs, "Sector");
			XARC_SER_KEY(xs, "x", i);
			XARC_SER_KEY(xs, "y", j);
			if (circuitSim[j*NUM_SECTORS + i]) {
				circuitSim[j*NUM_SECTORS + i]->Serialize(xs);
			}
			if (fluidSim[j*NUM_SECTORS + i]) {
				fluidSim[j*NUM_SECTORS + i]->Unsettle();		// doesn't have to keep state around.
			}
			XarcClose(xs);
		}
	}
	XarcClose(xs);
}

void PhysicsSims::DoTick(int delta)
{
	{
		PROFILE_BLOCK(FluidSim);
		int n = fluidTicker.Delta(delta);
		while (n--) {
			if (fluidSim[fluidSector]) {
				fluidSim[fluidSector]->DoStep();
			}
			fluidSector++;
			if (fluidSector >= Square(NUM_SECTORS)) {
				fluidSector = 0;
			}
		}
	}

	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		if (fluidSim[i]) {
			fluidSim[i]->EmitWaterfalls(delta, context->engine);
		}
	}
	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		if (circuitSim[i]) {
			circuitSim[i]->DoTick(int(delta));
		}
	}
}