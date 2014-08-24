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
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GameItem* mainItem = parentChit->GetItem();
	Vector2I sector = spatial->GetSector();
	CoreScript* cs = CoreScript::GetCore(sector);

	// Create workers, if needed.
	Rectangle2F b = ToWorld(InnerSectorBounds(sector));
	CChitArray arr;
	ItemNameFilter workerFilter(IStringConst::worker, IChitAccept::MOB);
	Context()->chitBag->QuerySpatialHash(&arr, b, 0, &workerFilter);
	if (arr.Empty()) {
		if (mainItem->wallet.gold >= WORKER_BOT_COST) {
			Transfer(&ReserveBank::Instance()->bank, &mainItem->wallet, WORKER_BOT_COST);
			Context()->chitBag->NewWorkerChit(cs->ParentChit()->GetSpatialComponent()->GetPosition(), parentChit->Team());
		}
		return true;	// we should be buying workers, even if we can't.
	}
	return false;
}


bool DomainAI::BuildRoad()
{
	// Check a random road.
	int p4 = parentChit->random.Rand(4);
	bool issuedOrders = false;
	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	CoreScript* cs = CoreScript::GetCore(sector);
	GLASSERT(cs);
	if (!cs) return false;
	WorkQueue* workQueue = cs->GetWorkQueue();
	GLASSERT(workQueue);
	if (!workQueue) return false;

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


int DomainAI::DoTick(U32 delta)
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if (!spatial) return ticker.Next();

	if (ticker.Delta(delta)) {

		Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
		CoreScript* cs = CoreScript::GetCore(sector);
		GLASSERT(cs);
		if (!cs) return VERY_LONG_TICK;
		WorkQueue* workQueue = cs->GetWorkQueue();
		GLASSERT(workQueue);
		if (!workQueue) return VERY_LONG_TICK;
		if (workQueue->HasJob()) {
			return ticker.Next();
		}

		while (true) {
			if (BuyWorkers()) break;
			if (BuildRoad()) break;
			break;
		}
	}
	return ticker.Next();
}
