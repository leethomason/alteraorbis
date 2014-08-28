#include "domainai.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../script/corescript.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"
#include "../game/gameitem.h"
#include "../game/reservebank.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../script/buildscript.h"
#include "../game/workqueue.h"

using namespace grinliz;

DomainAI::DomainAI()
{

}

DomainAI::~DomainAI()
{

}

void DomainAI::OnAdd(Chit* chit, bool initialize)
{
	super::OnAdd(chit, initialize);
	ticker.SetPeriod(2000 + chit->random.Rand(1000));

	// Computer the roads so that we have them later.
	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	const SectorData& sectorData = Context()->worldMap->GetSector(sector);
	for (int i = 0; i < 4; ++i) {
		int port = 1 << i;
		if (sectorData.ports & port) {
			Rectangle2I portLoc = sectorData.GetPortLoc(port);
			Vector2I it = portLoc.Center();
			while (it != sectorData.core) {
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it);
				if (!wg.IsPort()) {
					road[i].Push(it);
				}
				it = it + wg.Path(0);
			}
			// Road goes from core to port:
			road[i].Reverse();
		}
	}
}


void DomainAI::OnRemove()
{
	super::OnRemove();
}


void DomainAI::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	this->EndSerialize(xs);
}


void DomainAI::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
}


bool DomainAI::BuyWorkers()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	if (!Preamble(&sector, &cs, &workQueue))
		return false;
	GameItem* mainItem = parentChit->GetItem();

	// Create workers, if needed.
	Rectangle2F b = ToWorld(InnerSectorBounds(sector));
	CChitArray arr;
	ItemNameFilter workerFilter(IStringConst::worker, IChitAccept::MOB);
	Context()->chitBag->QuerySpatialHash(&arr, b, 0, &workerFilter);
	static const int GOLD[4] = { WORKER_BOT_COST, 400, 800, 1200 };
	for (int i = 0; i < 4; ++i) {
		if (arr.Size() == i && mainItem->wallet.gold >= GOLD[i]) {
			Transfer(&ReserveBank::Instance()->bank, &mainItem->wallet, WORKER_BOT_COST);
			Context()->chitBag->NewWorkerChit(cs->ParentChit()->GetSpatialComponent()->GetPosition(), parentChit->Team());
			return true;	// we should be buying workers, even if we can't.
		}
	}
	return false;
}


bool DomainAI::Preamble(grinliz::Vector2I* sector, CoreScript** cs, WorkQueue** wq)
{
	GLASSERT(parentChit->GetSpatialComponent());
	*sector = parentChit->GetSpatialComponent()->GetSector();
	*cs = CoreScript::GetCore(*sector);
	GLASSERT(*cs);
	if (!(*cs)) return false;
	*wq = (*cs)->GetWorkQueue();
	GLASSERT(*wq);
	if (!(*wq)) return false;

	return true;
}


bool DomainAI::BuildRoad()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	if (!Preamble(&sector, &cs, &workQueue))
		return false;

	// Check a random road.
	int p4 = parentChit->random.Rand(4);
	bool issuedOrders = false;

	for (int i = 0; i < road[p4].Size(); ++i) {
		Vector2I pos2i = road[p4][i];
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);
		if (!wg.Pave() || wg.Plant() || wg.RockHeight()) {
			workQueue->AddAction(pos2i, BuildScript::PAVE);
			issuedOrders = true;
		}
	}
	return issuedOrders;
}


bool DomainAI::BuildBuilding(int id)
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	if (!Preamble(&sector, &cs, &workQueue))
		return false;

	BuildScript buildScript;
	const BuildData& bd = buildScript.GetData(id);

	int endIndex = 0;
	for (int i = 0; i < MAX_ROADS; ++i) {
		endIndex = Max(endIndex, road[i].Size());
	}

	for (int index = 2; index < endIndex; ++index) {
		for (int i = 0; i < MAX_ROADS; ++i) {
			if (index >= road[i].Size())
				continue;

			Vector2I head = road[i][index];
			Vector2I tail = road[i][index - 1];
			Vector2I dir = head - tail;
			Vector2I left  = { -dir.y, dir.x };
			Vector2I right = { dir.y, dir.x };
			Vector2I porchScoot = { 0, 0 };

			int size = bd.size;
			if (bd.porch) {
				++size;
			}

			int rotation = 0;
			if (dir.y == 1) {
				rotation = 270;
				porchScoot.Set(1, 0);
			}
			else if (dir.x == -1) {
				rotation = 180;
				porchScoot.Set(0, 1);
			}
			else if (dir.y == -1) {
				rotation = 90;
			}

			if (!bd.porch) {
				porchScoot.Zero();
			}

			Vector2I loc0, loc1;
			loc0 = head + left;
			loc1 = head + left * (size) + dir*(size-1);
			Rectangle2I b;
			b.FromPair(loc0, loc1);

			bool okay = true;
			for (Rectangle2IIterator it(b); !it.Done(); it.Next()) {
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
				if (!wg.IsLand() || wg.Pave()) {
					okay = false;
					break;
				}
			}

			if (okay) {
				Chit* chit = Context()->chitBag->QueryBuilding(b);
				okay = !chit;
			}

			if (okay) {
				/*
				if (bd.porch) {
					Vector2I p1 = head + left;
					Vector2I p2 = p1;
					if (dir.x == 0) {
						p1.y = b.min.y;
						p2.y = b.max.y;
					}
					else {
						p1.x = b.min.x;
						p2.x = b.max.x;
					}
					workQueue->AddAction(p1, BuildScript::PAVE);
					if (size > 1) {
						workQueue->AddAction(p2, BuildScript::PAVE);						
					}
				}*/
				workQueue->AddAction(b.min + porchScoot, id, (float)rotation);
				return true;
			}
		}
	}
	return false;
}


int DomainAI::DoTick(U32 delta)
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if (!spatial) return ticker.Next();

	if (ticker.Delta(delta)) {
		Vector2I sector = { 0, 0 };
		CoreScript* cs = 0;
		WorkQueue* workQueue = 0;
		if (!Preamble(&sector, &cs, &workQueue))
			return VERY_LONG_TICK;

		// FIXME: this isn't really correct. will stall
		// domain ai if there is an inaccessible job.
		if (workQueue->HasJob()) {
			return ticker.Next();
		}

		int arr[BuildScript::NUM_TOTAL_OPTIONS] = { 0 };
		Context()->chitBag->BuildingCounts(sector, arr, BuildScript::NUM_TOTAL_OPTIONS);

		while (true) {
			if (BuyWorkers()) break;
			if (BuildRoad()) break;
			if (arr[BuildScript::TROLL_STATUE] == 0 && BuildBuilding(BuildScript::TROLL_STATUE)) break;
			if (arr[BuildScript::MARKET] < 4 && BuildBuilding(BuildScript::MARKET)) break;
			break;
		}
	}
	return ticker.Next();
}
