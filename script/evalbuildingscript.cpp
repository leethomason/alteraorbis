#include "evalbuildingscript.h"
#include "plantscript.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../game/mapspatialcomponent.h"
#include "../game/worldmap.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;

void EvalBuildingScript::Serialize(XStream* xs)
{
	XarcOpen(xs, ScriptName());
	XarcClose(xs);

	// Use this opportunity to spread timers out.
	timer.Randomize(scriptContext->chit->ID());
}


double EvalBuildingScript::EvalIndustrial( bool debugLog )
{
	Chit* building = ParentChit();
	int count = 0;
	double scale = 0;
	int hit = 0;
	int hitB = 0, hitIBuilding = 0, hitNBuilding = 0, hitWater = 0, hitPlant = 0, hitRock = 0;
	int nRays = 0;


	if (lastEval == 0 || (scriptContext->chitBag->AbsTime() - lastEval) > 2000) {
		lastEval = scriptContext->chitBag->AbsTime();

		GameItem* item = building->GetItem();
		GLASSERT(item);

		IString consume = item->keyValues.GetIString("zoneConsume");
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
		bounds.Outset(RAD);

		WorldMap* worldMap = scriptContext->chitBag->GetContext()->worldMap;
		Rectangle2I mapBounds = worldMap->Bounds();
		if (!mapBounds.Contains(bounds)) {
			eval = 0;
			return eval;	// not worth dealing with edge of world
		}
		CChitArray arr;
		BuildingFilter buildingFilter;
		PlantFilter plantFilter;
		int hasWaterfalls = worldMap->ContainsWaterfall(bounds);

		LumosChitBag* chitBag = building->GetLumosChitBag();
		Rectangle2IEdgeIterator it(bounds);

		while (!it.Done()) {
			++count;
			double s = 0;

			Vector2I pos = { it.Pos().x >= porch.max.x ? porch.max.x : porch.min.x,
				it.Pos().y >= porch.max.y ? porch.max.y : porch.min.y };

			LineWalk walk(pos.x, pos.y, it.Pos().x, it.Pos().y);
			walk.Step();	// ignore where we are standing.

			while (walk.CurrentStep() <= walk.NumSteps()) {	// non-intuitive iterator. See linewalk docs.
				// - building
				// - plant
				// - ice
				// - rock
				// - waterfall
				// - water

				// Buildings. Can be 2x2. Extend out beyond current check.
				bool hitBuilding = false;
				Vector2I p = walk.P();
				chitBag->QuerySpatialHash(&arr, ToWorld2F(p), 0.8f, 0, &buildingFilter);
				for (int i = 0; i < arr.Size(); ++i) {
					MapSpatialComponent* buildingMSC = GET_SUB_COMPONENT(arr[i], SpatialComponent, MapSpatialComponent);
					GLASSERT(buildingMSC);
					if (buildingMSC->Bounds().Contains(p)) {
						hitBuilding = true;
						double thisSys = arr[i]->GetItem()->GetBuildingIndustrial(true);
						s += thisSys;
						hitB++;
						if (thisSys == -1) hitIBuilding++;
						if (thisSys == 1) hitNBuilding++;
						break;
					}
				}
				if (hitBuilding) break;

				chitBag->QuerySpatialHash(&arr, ToWorld2F(p), 0.1f, 0, &plantFilter);
				if (arr.Size()) {
					int type = 0, stage = 0;
					PlantScript::IsPlant(arr[0], &type, &stage);
					s -= double(stage + 1) / double(PlantScript::NUM_STAGE);
					if (stage >= 2) break;
					++hitPlant;
				}

				const WorldGrid& wg = worldMap->GetWorldGrid(p.x, p.y);
				if (wg.RockHeight()) {
					if (wg.RockType() == WorldGrid::ROCK) {
						s -= 0.1;
					}
					++hitRock;
					break;
				}
				if (wg.IsWater()) {
					s -= 1.0;	// / double(RAD);
					++hitWater;
					break;
				}

				Rectangle2I wb;
				wb.min = wb.max = p;
				if (hasWaterfalls && worldMap->ContainsWaterfall(wb)) {
					s -= 10.0;
					break;
				}
				walk.Step();
			}
			if (walk.CurrentStep() <= walk.NumSteps()) {
				++nRays;
			}

			scale += Clamp(s, -1.0, 1.0);

			// double move - don't need that much accuracy
			it.Next();
			it.Next();
		}
		eval = scale / double(nRays);
	}
	if (debugLog) {
		Vector2I pos = building->GetSpatialComponent()->GetPosition2DI();
		GLOUTPUT(("Building %s at %d,%d eval=%.2f scale=%.2f nRays=%d count=%d\n  hit: Build=%d (I=%d N=%d) water=%d plant=%d rock=%d\n",
			building->GetItem()->Name(),
			pos.x, pos.y, 
			eval,
			scale, nRays, count,
			hitB, hitIBuilding, hitNBuilding, hitWater, hitPlant, hitRock));
	}

	return eval;
}


int EvalBuildingScript::DoTick(U32 delta)
{
	if (timer.Delta(delta)) {
		MapSpatialComponent* msc = GET_SUB_COMPONENT(scriptContext->chit, SpatialComponent, MapSpatialComponent);
		if (msc) {
			msc->UpdatePorch();
		}
	}
	return timer.Next();
}


