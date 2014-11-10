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
#include "../game/mapspatialcomponent.h"
#include "../Shiny/include/Shiny.h"
#include "roadai.h"

using namespace grinliz;


DomainAI* DomainAI::Factory(int team)
{
	switch (Team::Group(team)) {
		case TEAM_TROLL:	return new TrollDomainAI();
		case TEAM_GOB:		return new GobDomainAI();
		case TEAM_KAMAKIRI:	return new KamakiriDomainAI();
		default:
		break;
	}
	GLASSERT(0);
	return 0;
}


DomainAI::DomainAI()
{
	GLASSERT(MAX_ROADS == RoadAI::MAX_ROADS);
	for (int i = 0; i < MAX_ROADS; ++i) {
		buildDistance[i] = 0;
	}
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
	roads = new RoadAI(sector.x*SECTOR_SIZE, sector.y*SECTOR_SIZE);
	CDynArray<Vector2I> road;

	for (int i = 0; i < 4; ++i) {
		int port = 1 << i;
		if (sectorData.ports & port) {
			Rectangle2I portLoc = sectorData.GetPortLoc(port);
			Vector2I it = portLoc.Center();
			road.Clear();
			while (it != sectorData.core) {
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it);
				if (!wg.IsPort()) {
					road.Push(it);
				}
				it = it + wg.Path(0);
			}
			// Road goes from core to port:
			road.Reverse();
			roads->AddRoad(road.Mem(), road.Size());
		}
	}
}


void DomainAI::OnRemove()
{
	super::OnRemove();
	delete roads;
}


void DomainAI::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	XARC_SER_ARR(xs, buildDistance, MAX_ROADS);
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
		if (arr.Size() == i && mainItem->wallet.Gold() >= GOLD[i]) {
			ReserveBank::GetWallet()->Deposit( &mainItem->wallet, WORKER_BOT_COST);
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


bool DomainAI::BuildRoad(int roadID, int d)
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;

	// Check a random road.
	bool issuedOrders = false;

	int n = 0;
	const Vector2I* road = roads->GetRoad(roadID, &n);
	n = Min(n, d);
	buildDistance[roadID] = Max(buildDistance[roadID], n);

	for (int i = 0; i < n; ++i) {
		Vector2I pos2i = road[i];
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);
		if ( (wg.Pave() != pave) || wg.Plant() || wg.RockHeight()) {
			workQueue->AddAction(pos2i, BuildScript::PAVE, 0, pave);
			issuedOrders = true;
		}
	}
	return issuedOrders;
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

	for (int p = 0; p < RoadAI::MAX_ROADS && !issuedOrders; ++p) {
		int n = 0;
		const Vector2I* road = roads->GetRoad(p, &n);
		if (road) {
			issuedOrders = BuildRoad(p, INT_MAX);
		}
	}
	return issuedOrders;
}


bool DomainAI::BuildPlaza()
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

	for (int i = 0; i < RoadAI::MAX_PLAZA; ++i) {
		const Rectangle2I* r = roads->GetPlaza(i);
		if (r) {
			for (Rectangle2IIterator it(*r); !it.Done(); it.Next()) {
				if (it.Pos() == sd.core) {
					continue;
				}

				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
				if ( wg.IsLand() && (wg.Pave() != pave) || wg.Plant() || wg.RockHeight()) {
					workQueue->AddAction(it.Pos(), BuildScript::PAVE, 0, pave);
					issuedOrders = true;
				}
			}
		}
	}
	return issuedOrders;
}


bool DomainAI::CanBuild(const grinliz::Rectangle2I& r)
{
	bool okay = true;
	WorldMap* worldMap = Context()->worldMap;
	for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
		const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos());
		if (!wg.IsLand()) {
			okay = false;
			break;
		}
	}
	return okay;
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

	for (roads->Begin(); !roads->Done(); roads->Next()) {
		int nZones = 0;
		const RoadAI::BuildZone* zones = roads->CalcZones(&nZones, bd.size, bd.porch);

		for (int i = 0; i < nZones; ++i) {

			const RoadAI::BuildZone& zone = zones[i];

			// Check that we aren't building too close to a farm.
			Rectangle2I farmCheckBounds = zone.fullBounds;
			farmCheckBounds.Outset(FARM_GROW_RAD);
			if (Context()->chitBag->QueryBuilding(ISC::farm, farmCheckBounds, 0)) {
				continue;	// Don't build too close to farms!
			}

			// A little teeny tiny effort to group natural and industrial buildings.
			if (bd.zone) {
				IString avoid = (bd.zone == BuildData::ZONE_INDUSTRIAL) ? ISC::natural : ISC::industrial;
				Rectangle2I conflictBounds = zone.fullBounds;
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

			bool okay = CanBuild(zone.fullBounds);
			if (okay) {
				Chit* chit = Context()->chitBag->QueryBuilding(IString(), zone.fullBounds, 0);
				okay = !chit;
				if (okay) {
					for (Rectangle2IIterator it(zone.fullBounds); !it.Done(); it.Next()) {
						GLASSERT(!Context()->chitBag->QueryBuilding(IString(), it.Pos(), 0));
						const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
						if (wg.Porch() || wg.IsPort()) {
							okay = false;
							break;
						}
						GLASSERT(!Context()->chitBag->QueryPorch(it.Pos(), 0));
					}
				}
			}

			if (okay) {
				GLASSERT(roads->IsOnRoad(zone.buildBounds, !zone.porchBounds.min.IsZero(), zone.rotation, false));

				BuildRoad(zone.roadID, zone.roadDistance);
				workQueue->AddAction(zone.buildBounds.min, id, (float)zone.rotation, 0);
				if (bd.porch) {
					for (Rectangle2IIterator it(zone.porchBounds); !it.Done(); it.Next()) {
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

	int bestScore = 0;
	RoadAI::BuildZone bestZone;
	Vector2I center = cs->ParentChit()->GetSpatialComponent()->GetPosition2DI();

	for (roads->Begin(7); !roads->Done(); roads->Next()) {

		int nZones = 0;
		const RoadAI::BuildZone* zones = roads->CalcFarmZones(&nZones);
		
		for (int i = 0; i < nZones; ++i) {
			const RoadAI::BuildZone& zone = zones[i];

			Rectangle2I extendedBounds = zone.fullBounds;
			extendedBounds.Outset(2);
			Rectangle2I clearBounds = zone.porchBounds;
			clearBounds.DoUnion(zone.buildBounds);

			// Can we build the pave and the solar farm?
			bool okay = true;
			if (CanBuild(zone.fullBounds) && !Context()->chitBag->QueryBuilding(IString(), extendedBounds, 0)) {
				int score = 0;
				for (Rectangle2IIterator it(zone.fullBounds); !it.Done(); it.Next()) {
					if (!clearBounds.Contains(it.Pos())) {
						const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
						if (wg.Plant()) {
							int stage = wg.PlantStage() + 1;
							score += stage * stage;
						}
						if (wg.IsPort()) {
							okay = false;
						}
					}
				}

				Vector2I d = center - zone.buildBounds.min;
				int len = Min(abs(d.x), abs(d.y));
				score = score - len;	// closer in farms are more efficient.

				// score>10 prevents lots of super-low value farms.
				if (okay && score > 10 && score > bestScore) {
					bestScore = score;
					bestZone = zone;
				}
			}
		}
	}
	if (bestScore) {
		GLASSERT(roads->IsOnRoad(bestZone.buildBounds, false, bestZone.rotation, true));
		BuildRoad(bestZone.roadID, bestZone.roadDistance);
		workQueue->AddAction(bestZone.buildBounds.min, BuildScript::FARM, (float)bestZone.rotation, 0);
		for (Rectangle2IIterator it(bestZone.porchBounds); !it.Done(); it.Next()) {
			workQueue->AddAction(it.Pos(), BuildScript::PAVE, 0, pave);
		}
		return true;
	}
	return false;
}


bool DomainAI::ClearDisconnected()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;

	// FIXME: handle case where there are more than 32 buildings. Cache dynarray?
	CChitArray arr;
	Rectangle2I inner = InnerSectorBounds(parentChit->GetSpatialComponent()->GetSector());
	Context()->chitBag->QueryBuilding(IString(), inner, &arr);

	GL_FOR_EACH_BEGIN(Chit*, chit, arr) {
		MapSpatialComponent* msc = chit->GetSpatialComponent()->ToMapSpatialComponent();
		if (msc && chit->GetItem()) {
			if (!roads->IsOnRoad(msc, chit->GetItem()->IName() == ISC::farm)) {
				workQueue->AddAction(msc->Bounds().min, BuildScript::CLEAR, 0, 0);
				return true;
			}
		}
	} GL_FOR_EACH_END
	return false;
}


bool DomainAI::ClearRoadsAndPorches()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return false;

	bool issued = false;

	// Clear out the roads.
	for (int i = 0; i < MAX_ROADS; ++i) {
		int n = 0;
		const Vector2I* road = roads->GetRoad(i, &n);
		n = Min(n, buildDistance[i]);

		for (int k = 0; road && k < n; ++k) {
			Vector2I pos = road[k];
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos);
			if (wg.IsLand()) {
				if (!wg.Pave() || wg.Plant() || wg.RockHeight()) {
					workQueue->AddAction(pos, BuildScript::PAVE, 0, pave);
					issued = true;
				}
			}
		}
	}

	// Clear out the porches.
	Rectangle2I bounds = SectorBounds(sector);
	CChitArray arr;
	Context()->chitBag->QueryBuilding(IString(), bounds, &arr);
	for (int i = 0; i < arr.Size(); ++i) {
		MapSpatialComponent* msc = GET_SUB_COMPONENT(arr[i], SpatialComponent, MapSpatialComponent);
		Rectangle2I pb;
		pb.SetInvalid();

		if (msc && arr[i]->GetItem() && arr[i]->GetItem()->IName() == ISC::farm) {
			Vector2I normal = WorldRotationToNormal(LRint(msc->GetYRotation()));
			Vector2I p0 = msc->GetPosition2DI() + normal;
			Vector2I p1 = msc->GetPosition2DI() + normal * FARM_GROW_RAD;
			pb.FromPair(p0, p1);
		}
		else if (msc && msc->HasPorch()) {
			pb = msc->PorchPos();
		}

		if (pb.IsValid()) {
			for (Rectangle2IIterator it(pb); !it.Done(); it.Next()) {
				Vector2I pos = it.Pos();
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos);
				if (wg.IsLand()) {
					if (!wg.Pave() || wg.Plant() || wg.RockHeight()) {
						workQueue->AddAction(pos, BuildScript::PAVE, 0, pave);
						issued = true;
					}
				}
			}
		}
	}
	return issued;
}


int DomainAI::DoTick(U32 delta)
{
	PROFILE_FUNC();
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
		int gold = parentChit->GetItem()->wallet.Gold();
		for (int i = 0; i < cs->NumCitizens(); ++i) {
			Chit* citizen = cs->CitizenAtIndex(i);
			if (citizen->GetItem()) {
				int citizenGold = citizen->GetItem()->wallet.Gold();
				if (citizenGold > gold / 4) {
					int tax = (citizenGold - gold / 4) / 4;	// brutal taxation every 10s. But keep core funded,	or we all go down together.
					if (tax > 0) {
						parentChit->GetWallet()->Deposit( &citizen->GetItem()->wallet, tax);
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


float DomainAI::CalcFarmEfficiency(const grinliz::Vector2I& sector)
{
	CChitArray farms;
	Context()->chitBag->QueryBuilding(ISC::farm, InnerSectorBounds(sector), &farms);

	float eff = 0;
	for (int i = 0; i < farms.Size(); ++i) {
		FarmScript* farmScript = (FarmScript*) farms[i]->GetComponent("FarmScript");
		if (farmScript) {
			eff += farmScript->Efficiency();
		}
	}
	return eff;
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
	super::OnAdd(chit, initialize);
	forgeTicker.SetPeriod(20 * 1000 + chit->random.Rand(1000));

	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	const SectorData& sectorData = Context()->worldMap->GetSector(sector);

	Vector2I c = sectorData.core;
	Rectangle2I plaza0;
	plaza0.FromPair(c.x - 2, c.y - 2, c.x + 2, c.y + 2);
	roads->AddPlaza(plaza0);
}


void TrollDomainAI::OnRemove()
{
	return super::OnRemove();
}


int TrollDomainAI::DoTick(U32 delta)
{
	// Skim off the reserve bank:
	GameItem* item = parentChit->GetItem();
	if (!parentChit->Destroyed() && item->wallet.Gold() < 500 && ReserveBank::GetWallet()->Gold() > 500) {
		item->wallet.Deposit(ReserveBank::GetWallet(), 500);
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

			TransactAmt cost;
			GameItem* item = ForgeScript::DoForge(itemType, -1, ReserveBank::Instance()->wallet, &cost, partsMask, effectsMask, tech, level, seed);
			if (item) {
				if (ReserveBank::GetWallet()->CanWithdraw(cost)) {

					item->wallet.Deposit(ReserveBank::GetWallet(), cost);
					market->GetItemComponent()->AddToInventory(item);

					// Mark this item as important with a destroyMsg:
					item->SetSignificant(Context()->chitBag->GetNewsHistory(), pos, NewsEvent::FORGED, NewsEvent::UN_FORGED,
										 Context()->chitBag->GetDeity(LumosChitBag::DEITY_TRUULGA));
				}
				else {
					delete item;
				}
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
		if (ClearDisconnected()) break;
		if (ClearRoadsAndPorches()) break;
		if (BuildPlaza()) break;
		if (arr[BuildScript::TROLL_STATUE] == 0 && BuildBuilding(BuildScript::TROLL_STATUE)) break;
		if ((arr[BuildScript::MARKET] < 2) && BuildBuilding(BuildScript::MARKET)) break;
		if ((arr[BuildScript::TROLL_BRAZIER] < nBraziers) && BuildBuilding(BuildScript::TROLL_BRAZIER)) break;
		if (BuildRoad()) break;	// will return true until all roads are built.
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
	super::OnAdd(chit, initialize);

	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	const SectorData& sectorData = Context()->worldMap->GetSector(sector);

	Vector2I c = sectorData.core;
	Rectangle2I plaza0;
	plaza0.FromPair(c.x - 2, c.y - 2, c.x + 2, c.y + 2);
	roads->AddPlaza(plaza0);
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

	float eff = CalcFarmEfficiency(sector);

	do {
		if (BuyWorkers()) break;
		if (ClearDisconnected()) break;
		if (ClearRoadsAndPorches()) break;
		if (BuildPlaza()) break;
		if (arr[BuildScript::SLEEPTUBE] < 4 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
		if (arr[BuildScript::FARM] == 0 && BuildFarm()) break;
		if (arr[BuildScript::DISTILLERY] < 1 && BuildBuilding(BuildScript::DISTILLERY)) break;
		if (arr[BuildScript::BAR] < 1 && BuildBuilding(BuildScript::BAR)) break;
		if (arr[BuildScript::MARKET] < 1 && BuildBuilding(BuildScript::MARKET)) break;
		if (arr[BuildScript::FORGE] < 1 && BuildBuilding(BuildScript::FORGE)) break;
		if (eff < 2.0f && BuildFarm()) break;
		if (eff > 1.5f) {
			if (arr[BuildScript::SLEEPTUBE] < 6 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
		}
		if (eff >= 2) {
			if (arr[BuildScript::EXCHANGE] < 1 && BuildBuilding(BuildScript::EXCHANGE)) break;
			if (arr[BuildScript::BAR] < 2 && BuildBuilding(BuildScript::BAR)) break;
			if (arr[BuildScript::SLEEPTUBE] < 8 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
			if (arr[BuildScript::VAULT] == 0 && BuildBuilding(BuildScript::VAULT)) break;	// collect Au from workers.
		}
	} while (false);
}


KamakiriDomainAI::KamakiriDomainAI()
{

}


KamakiriDomainAI::~KamakiriDomainAI()
{

}

void KamakiriDomainAI::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	super::Serialize( xs );
	this->EndSerialize( xs );
}


void KamakiriDomainAI::OnAdd(Chit* chit, bool initialize)
{
	super::OnAdd(chit, initialize);

	Vector2I sector = parentChit->GetSpatialComponent()->GetSector();
	const SectorData& sectorData = Context()->worldMap->GetSector(sector);

	Vector2I c = sectorData.core;
	Rectangle2I plaza0, plaza1;
	plaza0.FromPair(c.x - 3, c.y - 1, c.x + 3, c.y + 1);
	plaza1.FromPair(c.x - 1, c.y - 3, c.x + 1, c.y + 3);

	roads->AddPlaza(plaza0);
	roads->AddPlaza(plaza1);
}


void KamakiriDomainAI::OnRemove()
{
	return super::OnRemove();
}

void KamakiriDomainAI::DoBuild()
{
	Vector2I sector = { 0, 0 };
	CoreScript* cs = 0;
	WorkQueue* workQueue = 0;
	int pave = 0;
	if (!Preamble(&sector, &cs, &workQueue, &pave))
		return;

	int arr[BuildScript::NUM_TOTAL_OPTIONS] = { 0 };
	Rectangle2I sectorBounds = SectorBounds(sector);

	Context()->chitBag->BuildingCounts(sector, arr, BuildScript::NUM_TOTAL_OPTIONS);
	float eff = CalcFarmEfficiency(sector);

	CChitArray bars;
	Context()->chitBag->QueryBuilding(ISC::bar, sectorBounds, &bars);
	int nElixir = 0;
	for (int i = 0; i < bars.Size(); ++i) {
		ItemComponent* ic = bars[i]->GetItemComponent();
		if (ic) {
			nElixir += ic->NumCarriedItems(ISC::elixir);
		}
	}

	do {
		if (BuyWorkers()) break;		
		if (ClearDisconnected()) break;
		if (ClearRoadsAndPorches()) break;
		if (BuildPlaza()) break;
		if (arr[BuildScript::SLEEPTUBE] < 4 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
		if (arr[BuildScript::FARM] == 0 && BuildFarm()) break;
		if (arr[BuildScript::DISTILLERY] < 1 && BuildBuilding(BuildScript::DISTILLERY)) break;
		if (arr[BuildScript::BAR] < 1 && BuildBuilding(BuildScript::BAR)) break;
		if (arr[BuildScript::KAMAKIRI_STATUE] < 4 && BuildBuilding(BuildScript::KAMAKIRI_STATUE)) break;
		if (arr[BuildScript::MARKET] < 1 && BuildBuilding(BuildScript::MARKET)) break;
		if (arr[BuildScript::FORGE] < 1 && BuildBuilding(BuildScript::FORGE)) break;
		if (eff < 2.0f && BuildFarm()) break;

		// Check efficiency to curtail over-building.
		if (eff > 1 && nElixir > 4) {
			if (arr[BuildScript::SLEEPTUBE] < 6 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
			if (arr[BuildScript::GUARDPOST] < 2 && BuildBuilding(BuildScript::GUARDPOST)) break;
		}
		if (eff >= 2 && nElixir > 4) {
			if (arr[BuildScript::BAR] < 2 && BuildBuilding(BuildScript::BAR)) break;
			if (arr[BuildScript::SLEEPTUBE] < 8 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
			if (arr[BuildScript::EXCHANGE] < 1 && BuildBuilding(BuildScript::EXCHANGE)) break;
			if (arr[BuildScript::SLEEPTUBE] < 12 && BuildBuilding(BuildScript::SLEEPTUBE)) break;
			if (arr[BuildScript::VAULT] == 0 && BuildBuilding(BuildScript::VAULT)) break;	// collect Au from workers.
			if (BuildRoad()) break;	// will return true until all roads are built.
		}
	} while (false);
}

