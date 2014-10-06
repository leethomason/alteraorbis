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
#include "../script/farmscript.h"

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
	ticker.SetPeriod(10 * 1000 + chit->random.Rand(1000));

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
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;
	GameItem* mainItem = parentChit->GetItem();

	// Create workers, if needed.
	// Be aggressive; domains tend to fall 
	// apart before getting built.
	Rectangle2F b = ToWorld(InnerSectorBounds(sector));
	CChitArray arr;
	ItemNameFilter workerFilter(ISC::worker, IChitAccept::MOB);
	Context()->chitBag->QuerySpatialHash(&arr, b, 0, &workerFilter);
	static const int GOLD[4] = { WORKER_BOT_COST, WORKER_BOT_COST * 2, WORKER_BOT_COST*3, 1200 };
	for (int i = 0; i < 4; ++i) {
		if (arr.Size() == i && mainItem->wallet.gold >= GOLD[i]) {
			Transfer(&ReserveBank::Instance()->bank, &mainItem->wallet, WORKER_BOT_COST);
			Context()->chitBag->NewWorkerChit(cs->ParentChit()->GetSpatialComponent()->GetPosition(), parentChit->Team());
			return true;	// we should be buying workers, even if we can't.
		}
	}
	return false;
}


bool DomainAI::Preamble(grinliz::Vector2I* sector, CoreScript** cs, WorkQueue** wq, int *pave)
{
	GLASSERT(parentChit->GetSpatialComponent());
	*sector = parentChit->GetSpatialComponent()->GetSector();
	*cs = CoreScript::GetCore(*sector);
	GLASSERT(*cs);
	if (!(*cs)) return false;
	*wq = (*cs)->GetWorkQueue();
	GLASSERT(*wq);
	if (!(*wq)) return false;

	*pave = (*cs)->GetPave();

	return true;
}


bool DomainAI::BuildRoad()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;

	// Check a random road.
	bool issuedOrders = false;

	for (int p = 0; p < 4 && !issuedOrders; ++p) {
		for (int i = 0; i < road[p].Size(); ++i) {
			Vector2I pos2i = road[p][i];
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);
			if ( (wg.Pave() != pave) || wg.Plant() || wg.RockHeight()) {
				workQueue->AddAction(pos2i, BuildScript::PAVE, 0, pave);
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
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
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
		if ( (wg.Pave() != pave) || wg.Plant() || wg.RockHeight()) {
			workQueue->AddAction(it.Pos(), BuildScript::PAVE, 0, pave);
			issuedOrders = true;
		}
	}
	return issuedOrders;
}


bool DomainAI::CanBuild(const grinliz::Rectangle2I& r, int pave)
{
	bool okay = true;
	WorldMap* worldMap = Context()->worldMap;
	for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
		const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos());
		if (!wg.IsLand() || (wg.Pave() == pave)) {
			okay = false;
			break;
		}
	}
	return okay;
}


int DomainAI::RightVectorToRotation(const grinliz::Vector2I& right)
{
	int rotation = 0;
	if (right.y == 1) rotation = 180;
	else if (right.x == 1) rotation = 270;
	else if (right.y == -1) rotation = 0;
	else if (right.x == -1) rotation = 90;
	return rotation;
}

bool DomainAI::BuildBuilding(int id)
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
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

			// Mix up the head/tail left/right
			if (index & 1) {
				Swap(&head, &tail);
			}

			Vector2I dir = head - tail;
			Vector2I left  = { -dir.y, dir.x };
			Vector2I right = -left;
			int porch = bd.porch ? 1 : 0;

			Rectangle2I fullBounds, buildBounds, porchBounds;
			GLASSERT(bd.size == 1 || bd.size == 2);
			// For building, if the size is one, then the tail is at the location of the head.
			Vector2I btail = (bd.size == 2) ? tail : head;
			fullBounds.FromPair(head - left *(porch + bd.size), btail - left);
			buildBounds.FromPair(head - left*(porch + bd.size), btail - left*(1 + porch));
			porchBounds.FromPair(head - left, btail - left);

			// Check that we aren't building too close to a farm.
			Rectangle2I farmCheckBounds = fullBounds;
			farmCheckBounds.Outset(FARM_GROW_RAD);
			if (Context()->chitBag->QueryBuilding(ISC::farm, farmCheckBounds, 0)) {
				continue;	// Don't build too close to farms!
			}

			// A little teeny tiny effort to group natural and industrial buildings.
			if (bd.zone) {
				IString avoid = (bd.zone == BuildData::ZONE_INDUSTRIAL) ? ISC::natural : ISC::industrial;
				Rectangle2I conflictBounds = fullBounds;
				conflictBounds.Outset(3);
				CChitArray conflict;
				Context()->chitBag->QueryBuilding(IString(), conflictBounds, &conflict);
				bool hasConflict = false;

				for (int i = 0; i < conflict.Size(); ++i) {
					const GameItem* item = conflict[i]->GetItem();
					if (item && item->keyValues.GetIString(ISC::zone) == avoid) {
						hasConflict = true;
						break;
					}
				}
				if (hasConflict) continue;	// try another location. 
			}

			// Why right? The building is to the left, so it faces right.
			int rotation = RightVectorToRotation(right);

			bool okay = CanBuild(fullBounds, pave);
			if (okay) {
				Chit* chit = Context()->chitBag->QueryBuilding(IString(), fullBounds, 0);
				okay = !chit;
			}

			if (okay) {
				workQueue->AddAction(buildBounds.min, id, (float)rotation, 0);
				if (bd.porch) {
					for (Rectangle2IIterator it(porchBounds); !it.Done(); it.Next()) {
						workQueue->AddAction(it.Pos(), BuildScript::PAVE, 0, pave);
					}
				}
				return true;
			}
		}
	}
	return false;
}


bool DomainAI::BuildFarm()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;

	int endIndex = 0;
	for (int i = 0; i < MAX_ROADS; ++i) {
		endIndex = Max(endIndex, road[i].Size());
	}

	int bestScore = 0;
	Vector2I bestFarm = { 0, 0 }, bestRight = { 0, 0 };
	Rectangle2I bestPave;

	for (int index = 7; index < endIndex; ++index) {
		for (int i = 0; i < MAX_ROADS; ++i) {
			if (index >= road[i].Size())
				continue;

			Vector2I head = road[i][index];
			Vector2I tail = road[i][index - 1];

			// Mix up the head/tail left/right
			if (index & 0) {
				Swap(&head, &tail);
			}

			Vector2I dir = head - tail;
			Vector2I left = { -dir.y, dir.x };
			Vector2I right = -left;

			Vector2I solar = head - left * 3;
			Rectangle2I paveBounds;
			paveBounds.FromPair(head - left * 2, head - left);
			Rectangle2I fullBounds;
			fullBounds.min = fullBounds.max = solar;
			fullBounds.Outset(FARM_GROW_RAD);
			Rectangle2I buildBounds = paveBounds;
			buildBounds.DoUnion(solar);
			Rectangle2I extendedBounds = fullBounds;
			extendedBounds.Outset(2);

			// Can we build the pave and the solar farm?
			if (CanBuild(buildBounds, pave) && !Context()->chitBag->QueryBuilding(IString(), extendedBounds, 0)) {
				int score = 0;
				for (Rectangle2IIterator it(fullBounds); !it.Done(); it.Next()) {
					if (!buildBounds.Contains(it.Pos())) {
						const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
						if (wg.Plant()) {
							int stage = wg.PlantStage() + 1;
							score += stage * stage;
						}
					}
				}
				score = score / index;	// don't want it TOO far from center.

				if (score > bestScore) {
					bestScore = score;
					bestFarm = solar;
					bestPave = paveBounds;
					bestRight = right;
				}
			}
		}
	}
	if (bestScore) {
		int rotation = RightVectorToRotation(bestRight);
		workQueue->AddAction(bestFarm, BuildScript::FARM, (float)rotation, 0);
		for (Rectangle2IIterator it(bestPave); !it.Done(); it.Next()) {
			workQueue->AddAction(it.Pos(), BuildScript::PAVE, 0, pave);
		}
		return true;
	}
	return false;
}


int DomainAI::DoTick(U32 delta)
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if (!spatial || !parentChit->GetItem()) return ticker.Next();

	if (ticker.Delta(delta)) {
		Vector2I sector = { 0, 0 };
		CoreScript* cs = 0;
		WorkQueue* workQueue = 0;
		int pave = 0;
		if (!Preamble(&sector, &cs, &workQueue, &pave))
			return VERY_LONG_TICK;

		// The tax man!
		// Solves the sticky problem: "how do non-player domains fund themselves?"
		int gold = parentChit->GetItem()->wallet.gold;
		for (int i = 0; i < cs->NumCitizens(); ++i) {
			Chit* citizen = cs->CitizenAtIndex(i);
			if (citizen->GetItem()) {
				int citizenGold = citizen->GetItem()->wallet.gold;
				if (citizenGold > gold / 4) {
					int tax = (citizenGold - gold / 4) / 4;	// brutal taxation every 10s. But keep core funded,	or we all go down together.
					if (tax > 0) {
						Transfer(&parentChit->GetItem()->wallet, &citizen->GetItem()->wallet, tax);
					}
				}
			}
		}

		// FIXME: this isn't really correct. will stall
		// domain ai if there is an inaccessible job.
		if (workQueue->HasAssignedJob()) {
			return ticker.Next();
		}
		this->DoBuild();
	}
	return ticker.Next();
}


TrollDomainAI::TrollDomainAI()
{

}
	

TrollDomainAI::~TrollDomainAI()
{

}


void TrollDomainAI::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	super::Serialize( xs );
	this->EndSerialize( xs );
}


void TrollDomainAI::OnAdd(Chit* chit, bool initialize)
{
	return super::OnAdd(chit, initialize);
	forgeTicker.SetPeriod(20 * 1000 + chit->random.Rand(1000));
}


void TrollDomainAI::OnRemove()
{
	return super::OnRemove();
}


int TrollDomainAI::DoTick(U32 delta)
{
	// Skim off the reserve bank:
	GameItem* item = parentChit->GetItem();
	if (item->wallet.gold < 500 && ReserveBank::BankPtr()->gold > 500) {
		Transfer(&item->wallet, ReserveBank::BankPtr(), 500);
	}

	// Build stuff for the trolls to buy.
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
				item->SetSignificant(Context()->chitBag->GetNewsHistory(), pos, NewsEvent::FORGED, NewsEvent::UN_FORGED, 
									 Context()->chitBag->GetDeity(LumosChitBag::DEITY_TRUULGA));
			}
		}
	}
	return Min(forgeTicker.Next(), super::DoTick(delta));
}


void TrollDomainAI::DoBuild()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return;

	int arr[BuildScript::NUM_TOTAL_OPTIONS] = { 0 };
	Context()->chitBag->BuildingCounts(sector, arr, BuildScript::NUM_TOTAL_OPTIONS);
	int nBraziers = 4 + (parentChit->ID() % 4);

	do {
		if (BuyWorkers()) break;
		if (BuildRoad()) break;	// will return true until all roads are built.
		if (BuildPlaza(2)) break;
		if (arr[BuildScript::TROLL_STATUE] == 0 && BuildBuilding(BuildScript::TROLL_STATUE)) break;
		if ((arr[BuildScript::MARKET] < 2) && BuildBuilding(BuildScript::MARKET)) break;
		if ((arr[BuildScript::TROLL_BRAZIER] < nBraziers) && BuildBuilding(BuildScript::TROLL_BRAZIER)) break;
	} while (false);
}


GobDomainAI::GobDomainAI()
{

}


GobDomainAI::~GobDomainAI()
{

}

void GobDomainAI::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	super::Serialize( xs );
	this->EndSerialize( xs );
}


void GobDomainAI::OnAdd(Chit* chit, bool initialize)
{
	return super::OnAdd(chit, initialize);
}


void GobDomainAI::OnRemove()
{
	return super::OnRemove();
}

void GobDomainAI::DoBuild()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return;

	int arr[BuildScript::NUM_TOTAL_OPTIONS] = { 0 };
	Context()->chitBag->BuildingCounts(sector, arr, BuildScript::NUM_TOTAL_OPTIONS);
	CChitArray farms;
	Context()->chitBag->QueryBuilding(ISC::farm, sector, &farms);

	float eff = 0.0f;
	for (int i = 0; i < farms.Size(); ++i) {
		FarmScript* farmScript = (FarmScript*) farms[i]->GetComponent("FarmScript");
		eff += farmScript->Efficiency();
	}

	do {
		if (BuyWorkers()) break;
		if (BuildRoad()) break;	// will return true until all roads are built.
		if (BuildPlaza(2)) break;
		if (arr[BuildScript::SLEEPTUBE] < 4 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
		if (arr[BuildScript::FARM] == 0 && BuildFarm()) break;
		if (arr[BuildScript::DISTILLERY] < 1 && BuildBuilding(BuildScript::DISTILLERY)) break;
		if (arr[BuildScript::BAR] < 1 && BuildBuilding(BuildScript::BAR)) break;
		if (arr[BuildScript::MARKET] < 1 && BuildBuilding(BuildScript::MARKET)) break;
		if (arr[BuildScript::FORGE] < 1 && BuildBuilding(BuildScript::FORGE)) break;
		if (eff < 2.0f && BuildFarm()) break;
		if (arr[BuildScript::EXCHANGE] < 1 && BuildBuilding(BuildScript::EXCHANGE)) break;
		if (arr[BuildScript::SLEEPTUBE] < 8 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
		if (arr[BuildScript::VAULT] == 0 && BuildBuilding(BuildScript::VAULT)) break;	// collect Au from workers.
	} while (false);
}

