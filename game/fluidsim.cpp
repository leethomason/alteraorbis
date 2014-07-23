#include "worldmap.h"
#include "lumosmath.h"
#include "fluidsim.h"
#include "news.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

using namespace grinliz;

static const int WATERFALL_HEIGHT = 3;

static const int NDIR = 4;
static const Vector2I DIR[NDIR] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

S8 FluidSim::water[SECTOR_SIZE*SECTOR_SIZE];
U8 FluidSim::boundHeight[SECTOR_SIZE*SECTOR_SIZE];
S8 FluidSim::emitterDist[SECTOR_SIZE*SECTOR_SIZE];
grinliz::CArray<grinliz::Vector2I, FluidSim::PRESSURE> FluidSim::fill;


FluidSim::FluidSim(WorldMap* wm, const Rectangle2I& b) : worldMap(wm), bounds(b), settled(false)
{
}


FluidSim::~FluidSim()
{

}


void FluidSim::EmitWaterfalls(U32 delta, Engine* engine)
{
	ParticleDef pdWater = engine->particleSystem->GetPD("fallingWater");
	ParticleDef pdLava = engine->particleSystem->GetPD("fallingLava");
	ParticleDef pdMist = engine->particleSystem->GetPD("mist");

	for (int i = 0; i<waterfalls.Size(); ++i) {
		const Vector2I& wf = waterfalls[i];
		const WorldGrid& wg = worldMap->grid[worldMap->INDEX(wf)];

		for (int j = 0; j<NDIR; ++j) {
			Vector2I v = wf + DIR[j];
			const WorldGrid& altWG = worldMap->grid[worldMap->INDEX(v)];
			int type = 0;
			if (HasWaterfall(wg, altWG, &type)) {

				Vector3F v3 = { (float)wf.x + 0.5f, (float)altWG.FluidHeight(), (float)wf.y + 0.5f };
				Vector3F half = { (float)DIR[j].x*0.5f, 0.0f, (float)DIR[j].y*0.5f };
				Vector3F right = { half.z, 0, half.x };

				Rectangle3F r3;
				r3.FromPair(v3 + half*0.8f - right, v3 + half*0.8f + right);

				static const Vector3F DOWN = { 0, -1, 0 };
				if (type == WorldGrid::FLUID_WATER)
					engine->particleSystem->EmitPD(pdWater, r3, DOWN, delta);
				else
					engine->particleSystem->EmitPD(pdLava, r3, DOWN, delta);

				r3.min.y = r3.max.y = altWG.FluidHeight() - 0.2f;
				engine->particleSystem->EmitPD(pdMist, r3, V3F_UP, delta);
				r3.min.y = r3.max.y = 0.0f;
				engine->particleSystem->EmitPD(pdMist, r3, V3F_UP, delta);
			}
		}
	}
}



void FluidSim::Reset(int x, int y)
{
	worldMap->ResetPather(x, y);
}


U32 FluidSim::Hash()
{
	unsigned int h = 2166136261U;

	for (int j = bounds.min.y + 1; j < bounds.max.y; ++j) {
		for (int i = bounds.min.x + 1; i < bounds.max.x; ++i) {
			const WorldGrid& wg = worldMap->grid[worldMap->INDEX(i, j)];
			int value = wg.fluidHeight + 32 * wg.fluidEmitter + 256 * wg.fluidType + (i << 16) + (j << 24);
			h ^= value;
			h *= 16777619;
		}
	}
	return h;
}


bool FluidSim::HasWaterfall(const WorldGrid& wg, const WorldGrid& altWG, int* type)
{
	if (   (altWG.IsFluid() && wg.IsFluid() && altWG.fluidHeight >= wg.fluidHeight + WATERFALL_HEIGHT)
		|| (wg.IsWater() && altWG.fluidHeight >= WATERFALL_HEIGHT))
	{
		if (type) {
			*type = altWG.FluidType();
		}
		return true;
	}
	return false;
}


int FluidSim::ContainsWaterfalls(const grinliz::Rectangle2I& bounds) const
{
	int count = 0;
	for (int i = 0; i < waterfalls.Size(); ++i) {
		if (bounds.Contains(waterfalls[i])) {
			++count;
		}
	}
	return count;
}


grinliz::Vector2I FluidSim::MaxAdjacentWater(int i, int j)
{
	Vector2I v = { i, j };
	Rectangle2I b;
	b.Set(0, 0, SECTOR_SIZE - 1, SECTOR_SIZE - 1);

	int maxH = 0;

	for (int k = 0; k < NDIR; ++k) {
		Vector2I p = { i + DIR[k].x, j + DIR[k].y };
		if (b.Contains(p)) {
			if (water[p.y*SECTOR_SIZE + p.x] > maxH) {
				maxH = water[p.y*SECTOR_SIZE + p.x];
				v = p;
			}
		}
	}
	return v;
}


bool FluidSim::DoStep()
{
	if (settled) return true;

	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	memset(boundHeight, 255, SECTOR_SIZE*SECTOR_SIZE*sizeof(boundHeight[0]));
	memset(emitterDist, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(emitterDist[0]));

	emitters.Clear();
	waterfalls.Clear();
	U32 startHash = Hash();

	// First pass: find all the emitters, and if they are bounded.
	// Generates a potential height map for everything.

	// FIXME: handle magma vs. water

	for (int y = bounds.min.y + 1; y < bounds.max.y; ++y) {
		for (int x = bounds.min.x + 1; x < bounds.max.x; ++x) {
			Vector2I pos2i = { x, y };
			const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos2i)];
			if (wg.IsFluidEmitter() && wg.RockHeight() == 0) {
				emitters.Push(pos2i);
			}
		}
	}

	for (int h = 1; h <= MAX_ROCK_HEIGHT; ++h) {
		for (int e = 0; e < emitters.Size(); ++e) {

			Vector2I pos2i = emitters[e];
			int i = pos2i.x - bounds.min.x;
			int j = pos2i.y - bounds.min.y;

			const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos2i)];
			GLASSERT(wg.IsFluidEmitter());
			int emitterBias = wg.FluidType() ? -1 : 1;

			int bh = boundHeight[j*SECTOR_SIZE + i];
			if (bh == 255) bh = 0;

			// Build up in layers.
			if (bh == h-1) {
				if ( this->BoundCheck(pos2i, h, false, wg.FluidType() > 0))
				{
					// FIXME: 2nd pass for "waterfall bounded"
					// It is bounded!
					while (!fill.Empty()) {
						Vector2I v = fill.Pop();
						int bhIndex = (v.y - bounds.min.y) * SECTOR_SIZE + (v.x - bounds.min.x);
						if (boundHeight[bhIndex] == 255) boundHeight[bhIndex] = 0;
						boundHeight[bhIndex] = Max((int)boundHeight[bhIndex], h);

						int dist = fill.Size()+1;
						if (emitterDist[bhIndex])
							emitterDist[bhIndex] = Min(abs(emitterDist[bhIndex]), dist) * emitterBias;
						else
							emitterDist[bhIndex] = dist * emitterBias;
						GLASSERT(emitterDist[bhIndex]);
					}
				}
			}
		}
	}

	// 2nd pass: lift or lower water / magma
	for (int y = bounds.min.y + 1; y < bounds.max.y; ++y) {
		for (int x = bounds.min.x + 1; x < bounds.max.x; ++x) {

			int i = x - bounds.min.x;
			int j = y - bounds.min.y;
			if (boundHeight[j*SECTOR_SIZE + i] == 255) 
				continue;

			Vector2I pos2i = { x, y };
			int index = worldMap->INDEX(pos2i);
			WorldGrid* worldGrid = &worldMap->grid[index];
			int h = FLUID_PER_ROCK * boundHeight[j*SECTOR_SIZE + i];
			int d = emitterDist[j*SECTOR_SIZE + i];
			GLASSERT(d);

			if ((int)worldGrid->fluidHeight < h) {
				worldGrid->fluidHeight += 1;
			}
			else if ((int)worldGrid->fluidHeight > h) {
				worldGrid->fluidHeight -= 1;
			}
			if (!worldGrid->IsFluidEmitter()) {
				worldGrid->fluidType = d < 0 ? 1 : 0;
			}
		}
	}
	
#if 1
	PressureStep();
#endif

	for (Rectangle2IIterator it(bounds); !it.Done(); it.Next()) {
		Reset(it.Pos().x, it.Pos().y);

		const WorldGrid& wg = worldMap->grid[worldMap->INDEX(it.Pos())];
		for (int k = 0; k < NDIR; ++k) {
			const WorldGrid& altWG = worldMap->grid[worldMap->INDEX(it.Pos() + DIR[k])];
			if (HasWaterfall(wg, altWG, 0)) {
				waterfalls.Push(it.Pos());
				// Only need the initial location - not all the waterfalls.
				break;
			}
		}
	}

	U32 endHash = Hash();
	settled = (startHash == endHash);
	return settled;
}


void FluidSim::PressureStep()
{
	Rectangle2I aoe = bounds;
	aoe.Outset(-1);

	// FIXME aoe has to be entire region.
	memset(water, 0, SECTOR_SIZE*SECTOR_SIZE*sizeof(water[0]));
	for (Rectangle2IIterator it(aoe); !it.Done(); it.Next()) {
		int i = it.Pos().x - bounds.min.x;
		int j = it.Pos().y - bounds.min.y;
		water[j*SECTOR_SIZE + i] = worldMap->grid[worldMap->INDEX(it.Pos())].fluidHeight;
	}

	for (Rectangle2IIterator it(aoe); !it.Done(); it.Next()) {
		// Only effects grids NOT in the boundHeight

		int i = it.Pos().x - bounds.min.x;
		int j = it.Pos().y - bounds.min.y;

		int index = worldMap->INDEX(it.Pos());
		WorldGrid* wg = &worldMap->grid[index];

		if (wg->FluidSink() || wg->RockHeight()) {
			continue;
		}

		if (boundHeight[j*SECTOR_SIZE + i] == 255) {
			int maxH = 0;
			for (int k = 0; k < NDIR; ++k) {
				int h = water[(j + DIR[k].y)*SECTOR_SIZE + i + DIR[k].x];
				maxH = Max(h, maxH);
			}
			if (maxH > (int)wg->fluidHeight + 1)
				wg->fluidHeight++;
			else if ((int)wg->fluidHeight && (maxH <= (int)wg->fluidHeight))
				wg->fluidHeight--;
		}
	}
}


int FluidSim::FindEmitter(const grinliz::Vector2I& pos2i, bool nominal, bool magma)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos2i)];
	int result = 0;

	// Go up; can only be bounded at higher h if bounded at lower h.
	for (int hRock = 1; hRock <= MAX_ROCK_HEIGHT; hRock++) {
		if (hRock > wg.RockHeight()) {
			fill.Clear();
			int h = hRock * FLUID_PER_ROCK;
			int area = BoundCheck(pos2i, h / FLUID_PER_ROCK, nominal, magma);

			// Don't start an emitter unless it is enclosed.
			// "water patties" look pretty silly.
			//if (area > 5 && area < PRESSURE) {	// the area > 5 is aesthetically nice, but unexpected when building traps.
			if (area > 0 && area < PRESSURE) {
				result = hRock;
			}
			else {
				break;
			}
		}
	}
	return result;
}


int FluidSim::BoundCheck(const Vector2I& start, int h, bool nominal, bool magma )
{
	// Pressure from the emitter.
	flag.ClearAll();
	fill.Clear();

	stack.Clear();
	stack.Push(start);
	flag.Set(start.x - bounds.min.x, start.y - bounds.min.y);

	int nFalls = 0;

	while (!stack.Empty() && fill.HasCap()) {
		Vector2I p = stack.PopFront();	// Need to PopFront() to get 'round' areas and not go depth-first
		fill.Push(p);

		int index = worldMap->INDEX(p);
		const WorldGrid& wg = worldMap->grid[index];

		int wgRockHeight = nominal ? wg.NominalRockHeight() : wg.RockHeight();
		GLASSERT(wgRockHeight < h);
		if (wg.FluidSink()) {
			return 0;
		}

		int push = 0;
		int pass = 0;

		for (int k = 0; k < NDIR; ++k) {
			Vector2I q = p + DIR[k];
			Vector2I qLoc = { q.x - bounds.min.x, q.y - bounds.min.y };

			int qIndex = worldMap->INDEX(q);
			const WorldGrid& qwg = worldMap->grid[qIndex];
			int qRockHeight = nominal ? qwg.NominalRockHeight() : qwg.RockHeight();

			if (!qwg.IsLand()) {
				nFalls++;
			}
			else if (qRockHeight < h) {
				++pass;

				if (!flag.IsSet(qLoc.x, qLoc.y)) {
					stack.Push(q);
					++push;
					flag.Set(qLoc.x, qLoc.y);
				}
			}
		}
		/* Love this code...but moving to all fluids be fully
		   bounded in order to deal with pathing issues.
		// This is a bit of craziness. Narrow passages are
		// considered bounding. (And create waterfalls.)
		// Detect a narrow passage if there are 2 ways
		// out and one way is marked already. Then
		// undo to bound the fill.
		if (magma && push == 1 && pass == 2) {
			stack.Pop();
		}
		*/
	}
	int area = fill.Size();
	if (area && area < PRESSURE && ((nFalls-1) <= area / 8))
		return area;
	return 0;
}
