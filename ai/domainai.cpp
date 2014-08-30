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
#include "../xegame/itemcomponent.h"
#include "../script/forgescript.h"
#include "../game/team.h"
#include "../script/procedural.h"

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
	forgeTicker.SetPeriod(10 * 1000 + chit->random.Rand(1000));

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
	bool issuedOrders = false;

	for (int p = 0; p < 4 && !issuedOrders; ++p) {
		for (int i = 0; i < road[p].Size(); ++i) {
			Vector2I pos2i = road[p][i];
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);
			if (!wg.Pave() || wg.Plant() || wg.RockHeight()) {
				workQueue->AddAction(pos2i, BuildScript::PAVE);
				issuedOrders = true;
			}
		}
	}
	return issuedOrders;
}


bool DomainAI::BuildPlaza(int size)
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	if (!Preamble(&sector, &cs, &workQueue))
		return false;

	// Check a random road.
	bool issuedOrders = false;
	const SectorData& sd = Context()->worldMap->GetSector(sector);
	Rectangle2I r;
	r.min = r.max = sd.core;
	r.Outset(size);

	for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
		if (it.Pos() == sd.core) {
			continue;
		}

		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
		if ( !wg.Pave() || wg.Plant() || wg.RockHeight()) {
			workQueue->AddAction(it.Pos(), BuildScript::PAVE);
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
			Vector2I right = -left;
			int porch = bd.porch ? 1 : 0;

			Rectangle2I fullBounds, buildBounds, porchBounds;
			fullBounds.FromPair(head - left *(porch + bd.size), tail - left);
			buildBounds.FromPair(head - left*(porch + bd.size), tail - left*(1 + porch));
			porchBounds.FromPair(head - left, tail - left);

			// FIXME: move to a function and don't think about the weird
			// rotation system again. Strange sort of coordinate system.

			// Why right? The building is to the left, so it faces right.
			int rotation = 0;
			if (right.y == 1) rotation = 180;
			else if (right.x == 1) rotation = 270;
			else if (right.y == -1) rotation = 0;
			else if (right.x == -1) rotation = 90;

			bool okay = true;
			for (Rectangle2IIterator it(fullBounds); !it.Done(); it.Next()) {
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
				if (!wg.IsLand() || wg.Pave()) {
					okay = false;
					break;
				}
			}

			if (okay) {
				Chit* chit = Context()->chitBag->QueryBuilding(fullBounds);
				okay = !chit;
			}

			if (okay) {
				workQueue->AddAction(buildBounds.min, id, (float)rotation);
				if (bd.porch) {
					for (Rectangle2IIterator it(porchBounds); !it.Done(); it.Next()) {
						workQueue->AddAction(it.Pos(), BuildScript::PAVE);
					}
				}
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
			if (BuildRoad()) break;	// will return true until all roads are built.
			if (BuildPlaza(2)) break;
			if (arr[BuildScript::TROLL_STATUE] == 0 && BuildBuilding(BuildScript::TROLL_STATUE)) break;
			if (arr[BuildScript::MARKET] < 4 && BuildBuilding(BuildScript::MARKET)) break;
			break;
		}
	}
	if (forgeTicker.Delta(delta)) {
		Vector2I sector = parentChit->GetSpatialComponent()->GetSector();

		// find a market.
		// if has cap, make an item
		// add the item, transfer from reserve bank
		Vector2F pos = parentChit->GetSpatialComponent()->GetPosition2D();
		Chit* market = Context()->chitBag->FindBuilding(ISC::market, sector, &pos, LumosChitBag::RANDOM_NEAR, 0, 0);
		if (market && market->GetItemComponent() && market->GetItemComponent()->CanAddToInventory()) {

			int group = 0, id = 0;
			int itemType = ForgeScript::GUN;
			int partsMask = 0xff;
			int effectsMask = GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK;
			int tech = 0;
			int level = 0;
			int seed = parentChit->random.Rand();

			Team::SplitID(parentChit->Team(), &group, &id);

			switch (group) {
				case TEAM_TROLL:
				{
					itemType = parentChit->random.Rand(ForgeScript::NUM_ITEM_TYPES);
					if (itemType == ForgeScript::RING) partsMask = WeaponGen::TROLL_RING_PART_MASK;
					effectsMask = 0;
					tech = 0;
					level = 4;
				}
				break;

				default:
				GLASSERT(0);
				break;
			}

			Wallet cost;
			GameItem* item = ForgeScript::DoForge(itemType, ReserveBank::Instance()->bank, &cost, partsMask, effectsMask, tech, level, seed);
			if (item) {
				Transfer(&item->wallet, &ReserveBank::Instance()->bank, cost);
				market->GetItemComponent()->AddToInventory(item);

				// Mark this item as important with a destroyMsg:
				item->GetItem()->keyValues.Set("destroyMsg", NewsEvent::UN_FORGED);
				NewsHistory* history = Context()->chitBag->GetNewsHistory();
				NewsEvent news(NewsEvent::FORGED, pos, item, Context()->chitBag->GetDeity(LumosChitBag::DEITY_TRUULGA));
				history->Add(news);
			}
		}
	}
	return ticker.Next();
}
