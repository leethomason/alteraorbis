#include "evalbuildingscript.h"
#include "plantscript.h"
#include "corescript.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../game/mapspatialcomponent.h"
#include "../game/worldmap.h"
#include "../game/fluidsim.h"
#include "../game/worldinfo.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;

void EvalBuildingScript::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	EndSerialize(xs);
}


void EvalBuildingScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		// Use this opportunity to spread timers out.
		timer.Randomize(parentChit->ID());
	}
}


double EvalBuildingScript::EvalIndustrial( bool debugLog )
{
	Chit* building = ParentChit();

	int hitB = 0, hitIBuilding = 0, hitNBuilding = 0, hitWater = 0, hitPlant = 0, hitRock = 0, hitWaterfall = 0;
	int hitShrub = 0;	// doesn't terminate a ray.


	if (lastEval == 0 || (Context()->chitBag->AbsTime() - lastEval) > 2000) {
		lastEval = Context()->chitBag->AbsTime();

		GameItem* item = building->GetItem();
		GLASSERT(item);
		reachable = true;

		IString consume = item->keyValues.GetIString(ISC::zone);
		if (consume.empty()) {
			eval = 0;
			return eval;
		}
		MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
		GLASSERT(msc);
		if (!msc) {
			eval = 0;
			return eval;
		}
		Rectangle2I porch = msc->PorchPos();

		static const int RAD = 4;

		Rectangle2I bounds = porch;
		if (porch.min.x == 0) {	// shouldn't happen, but be sure.
			GLASSERT(0);
			eval = 0;
			return eval;	
		}

		bounds.Outset(RAD);
		Vector2I sector = ToSector(building->Position());

		WorldMap* worldMap = Context()->worldMap;
		Rectangle2I mapBounds = worldMap->Bounds();
		if (!mapBounds.Contains(bounds)) {
			eval = 0;
			return eval;	// not worth dealing with edge of world
		}

		// Check if we can go from the core to the porch.
		// And make sure the core is inUse!
		CoreScript* cs = CoreScript::GetCore(ToSector(porch.min));
		if (!cs || !cs->InUse())
			reachable = false;
		if (reachable) {
			const SectorData& sd = worldMap->GetSectorData(ToSector(porch.min));
			reachable = worldMap->CalcPath(ToWorld2F(sd.core), ToWorld2F(porch.min), 0, 0, false);
		}

		CChitArray arr;
		BuildingFilter buildingFilter;
		const FluidSim* fluidSim = worldMap->GetFluidSim(sector);
		bool hasWaterfalls = fluidSim->NumWaterfalls() > 0;

		LumosChitBag* chitBag = Context()->chitBag;
		Rectangle2IEdgeIterator it(bounds);

		while (!it.Done()) {
			Vector2I pos = { it.Pos().x >= porch.max.x ? porch.max.x : porch.min.x,
							 it.Pos().y >= porch.max.y ? porch.max.y : porch.min.y };

			LineWalk walk(pos.x, pos.y, it.Pos().x, it.Pos().y);
			walk.Step();	// ignore where we are standing.

			while ( !walk.Done() ) {	// non-intuitive iterator. See linewalk docs.
				// - building
				// - plant
				// - ice
				// - rock
				// - waterfall
				// - water

				// Buildings. Can be 2x2. Extend out beyond current check.
				bool hitBuilding = false;
				Vector2I p = walk.P();
				// Don't count self as a hit, but stops the ray cast.
				// Also, use a larger radius because buildings can be 2x2
				chitBag->QuerySpatialHash(&arr, ToWorld2F(p), 0.8f, 0, &buildingFilter);
				for (int i = 0; i < arr.Size(); ++i) {
					if (arr[i] != building) {
						MapSpatialComponent* buildingMSC = GET_SUB_COMPONENT(arr[i], SpatialComponent, MapSpatialComponent);
						GLASSERT(buildingMSC);
						if (buildingMSC->Bounds().Contains(p)) {
							hitBuilding = true;
							double thisSys = arr[i]->GetItem()->GetBuildingIndustrial();

							hitB++;
							if (thisSys <= -0.5) hitNBuilding++;
							if (thisSys >= 0.5)  hitIBuilding++;

							break;
						}
					}
				}
				if (hitBuilding) break;
				
				const WorldGrid& wg = worldMap->GetWorldGrid(p.x, p.y);
				if (wg.Plant()) {
					int type = wg.Plant() - 1;
					int stage = wg.PlantStage();

					if (stage >= 2) {
						++hitPlant;
						break;
					}
					else {
						hitShrub++;
					}
				}

				if (wg.RockHeight()) {
					++hitRock;
					break;
				}
				if (wg.IsWater()) {
					++hitWater;
					break;
				}

				Rectangle2I wb;
				wb.min = wb.max = p;
				if (hasWaterfalls && fluidSim->ContainsWaterfalls(wb)) {
					++hitWaterfall;
					break;
				}
				walk.Step();
			}
			it.Next();
		}

		// Note rock/ice isn't counted either way.
		int natural = hitNBuilding
			+ hitWater
			+ hitPlant
			+ 10 * hitWaterfall
			+ hitShrub / 4;				// small plants don't add to rRays, so divide is okay.
		int industrial = hitIBuilding;
		int nRays = hitNBuilding + hitWater + hitPlant + hitWaterfall + hitIBuilding;

		eval = 0;
		if (nRays) {
			// With this system, that one ray (say from a distillery to plant) can be
			// hugely impactful. This may need tweaking:
			if (nRays < 2)
				nRays = 2;
			eval = double(industrial - natural) / double(nRays);
		}
		eval = Clamp(eval, -1.0, 1.0);

		if (debugLog) {
			Vector2I pos = ToWorld2I(building->Position());
			GLOUTPUT(("Building %s at %d,%d eval=%.2f nRays=%d \n  hit: Build=%d (I=%d N=%d) water=%d plant=%d rock=%d\n",
				building->GetItem()->Name(),
				pos.x, pos.y,
				eval,
				nRays,
				hitB, hitIBuilding, hitNBuilding, hitWater, hitPlant, hitRock));
		}
	}
	return eval;
}


int EvalBuildingScript::DoTick(U32 delta)
{
	if (timer.Delta(delta)) {
		// Regular update to change the porch.
		parentChit->SendMessage(ChitMsg(ChitMsg::CHIT_POS_CHANGE));
	}
	return timer.Next();
}


