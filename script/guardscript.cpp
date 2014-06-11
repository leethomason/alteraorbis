#include "guardscript.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include "../game/lumosmath.h"
#include "../game/lumoschitbag.h"
#include "../game/team.h"
#include "../game/aicomponent.h"

using namespace grinliz;

static const int RAD = 2;
static const int NOTIFY_RAD = 8;

GuardScript::GuardScript() : timer(1000)
{
}


void GuardScript::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	timer.Serialize(xs, "timer");
	EndSerialize(xs);
}


void GuardScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		timer.Randomize(parentChit->random.Rand());
	}
}


int GuardScript::DoTick(U32 delta)
{
	if (timer.Delta(delta)) {
		// Check for enemies.
		Vector2I pos2i = parentChit->GetSpatialComponent()->GetPosition2DI();
		Vector2I sector = ToSector(pos2i);
		Rectangle2I innerSector = InnerSectorBounds(sector);

		Rectangle2I r;
		r.min = r.max = pos2i;
		r.Outset(RAD);
		r.DoIntersection(innerSector);

		Rectangle2F rf = ToWorld(r);

		CChitArray enemyArr;
		MOBIshFilter enemyFilter;
		CArray<int, 8> enemyID;

		enemyFilter.CheckRelationship(parentChit, RELATE_ENEMY);
		Context()->chitBag->QuerySpatialHash(&enemyArr, rf, 0, &enemyFilter);
		for (int i = 0; i < enemyArr.Size() && enemyID.HasCap(); ++i) {
			// convert to IDs for MakeAware()
			enemyID.Push(enemyArr[i]->ID());
		}

		if (enemyID.Size()) {
			r.min = r.max = pos2i;
			r.Outset(NOTIFY_RAD);
			r.DoIntersection(innerSector);
			rf = ToWorld(r);

			CChitArray friendArr;
			MOBIshFilter friendFilter;
			friendFilter.CheckRelationship(parentChit, RELATE_FRIEND);

			Context()->chitBag->QuerySpatialHash(&friendArr, rf, 0, &friendFilter);
			for (int i = 0; i < friendArr.Size(); ++i) {
				AIComponent* ai = friendArr[i]->GetAIComponent();
				if (ai) {
					ai->MakeAware(enemyID.Mem(), enemyID.Size());
				}
			}

		}
	}

	return timer.Next();
}

