#include "tasklist.h"
#include "marketai.h"

#include "../game/lumosmath.h"
#include "../game/pathmovecomponent.h"
#include "../game/workqueue.h"
#include "../game/aicomponent.h"
#include "../game/worldmap.h"
#include "../game/worldgrid.h"
#include "../game/lumoschitbag.h"
#include "../game/lumosgame.h"
#include "../game/team.h"
#include "../game/mapspatialcomponent.h"
#include "../game/reservebank.h"
#include "../game/circuitsim.h"
#include "../game/physicssims.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitcontext.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

#include "../script/corescript.h"
#include "../script/buildscript.h"
#include "../script/forgescript.h"
#include "../script/procedural.h"
#include "../script/plantscript.h"

#include "../engine/particle.h"

using namespace grinliz;
using namespace ai;

Task Task::RemoveTask( const grinliz::Vector2I& pos2i )
{
	return Task::BuildTask( pos2i, BuildScript::CLEAR, 0 );
}


Vector2I TaskList::Pos2I() const
{
	Vector2I v = { 0, 0 };
	if ( !taskList.Empty() ) {
		v = taskList[0].pos2i;
	}
	return v;
}


void Task::Serialize( XStream* xs )
{
	XarcOpen( xs, "Task" );
	XARC_SER( xs, action );
	XARC_SER( xs, buildScriptID );
	XARC_SER( xs, pos2i );
	XARC_SER( xs, timer );
	XARC_SER( xs, data );
	XarcClose( xs );
}


void TaskList::Push( const Task& task )
{
	taskList.Push( task );

	Vector2I sector = ToSector(task.pos2i);
	CoreScript* cs = CoreScript::GetCore(sector);
	if (cs) {
		cs->AddTask(task.pos2i);
	}
}


void TaskList::Remove()
{
	GLASSERT(taskList.Size() > 0);

	Vector2I sector = ToSector(taskList[0].pos2i);
	CoreScript* cs = CoreScript::GetCore(sector);
	if (cs) {
		cs->RemoveTask(taskList[0].pos2i);
	}

	taskList.Remove(0);
}


void TaskList::Clear()
{
	for (int i = 0; i < taskList.Size(); ++i) {
		Vector2I sector = ToSector(taskList[i].pos2i);
		CoreScript* cs = CoreScript::GetCore(sector);
		if (cs) {
			cs->RemoveTask(taskList[i].pos2i);
		}
	}
	taskList.Clear();
}

void TaskList::Serialize( XStream* xs )
{
	XarcOpen( xs, "TaskList" );
	XARC_SER_ARR( xs, buildingsUsed, NUM_BUILDING_USED);
	XARC_SER_CARRAY( xs, taskList );
	socialTicker.Serialize( xs, "socialTicker" );
	XarcClose( xs );
}


bool TaskList::DoStanding(int time)
{
	if (taskList.Empty()) return false;
	if (Standing()) {
		taskList[0].timer -= time;
		if (taskList[0].timer <= 0) {
			Remove();
		}
		int n = socialTicker.Delta(time);
		for (int i = 0; i < n; ++i) {
			SocialPulse(ToWorld2F(chit->Position()));
		}
		return true;
	}
	return false;
}


void TaskList::DoTasks(Chit* chit, U32 delta)
{
	if (taskList.Empty()) return;

	PathMoveComponent* pmc = GET_SUB_COMPONENT(chit, MoveComponent, PathMoveComponent);

	if (!pmc || !chit->GetAIComponent()) {
		Clear();
		return;
	}

	LumosChitBag* chitBag = chit->Context()->chitBag;
	Task* task = &taskList[0];
	Vector2I pos2i = ToWorld2I(chit->Position());
	Vector2I sector = ToSector(pos2i);
	//Vector2F taskPos2 = ToWorld2F(task->pos2i);
	//Vector3F taskPos3 = ToWorld3F(task->pos2i);
	//Rectangle2I innerBounds = InnerSectorBounds(sector);
	CoreScript* coreScript = CoreScript::GetCore(sector);
	Chit* controller = coreScript ? coreScript->ParentChit() : 0;

	switch (task->action) {
		case Task::TASK_MOVE:
		if (task->timer == 0) {
			// Need to start the move.
			Vector2F dest = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
			chit->GetAIComponent()->Move(dest, false);
			task->timer = delta;
		}
		else if (pmc->Stopped()) {
			if (pos2i == task->pos2i) {
				// arrived!
				Remove();
			}
			else {
				// Something went wrong.
				Clear();
			}
		}
		else {
			task->timer += delta;	// currently not used, but useful to see if we are taking too long.
		}
		break;

		case Task::TASK_STAND:
		if (pmc->Stopped()) {
			chit->GetAIComponent()->Stand();
			DoStanding(delta);

			if (taskList.Size() >= 2) {
				int action = taskList[1].action;
				if (action == Task::TASK_BUILD) {
					Vector3F pos = ToWorld3F(taskList[1].pos2i);
					pos.y = 0.5f;
					context->engine->particleSystem->EmitPD(ISC::construction, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
				else if (action == Task::TASK_USE_BUILDING) {
					Vector3F pos = chit->Position();
					context->engine->particleSystem->EmitPD(ISC::useBuilding, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
				else if (action == Task::TASK_REPAIR_BUILDING) {
					Vector3F pos = chit->Position();
					context->engine->particleSystem->EmitPD(ISC::repair, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
			}
		}
		break;

		case Task::TASK_REPAIR_BUILDING:
		{
			Chit* building = chitBag->GetChit(task->data);
			if (building) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
				if (msc) {
					Rectangle2I b = msc->Bounds();
					b.Outset(1);
					if (b.Contains(pos2i)) {
						GameItem* item = building->GetItem();
						if (item) {
							item->hp = double(item->TotalHP());
						}
						RenderComponent* rc = building->GetRenderComponent();
						if (rc) {
							rc->AddDeco("repair", STD_DECO);
						}
					}
				}
			}
			Remove();
		}
		break;

		case Task::TASK_BUILD:
		{
			// "BUILD" just refers to a BuildScript task. Do some special case handling
			// up front for rocks. (Which aren't handled by the general code.)
			// Although a small kludge, way better than things used to be.

			//const WorldGrid& wg = context->worldMap->GetWorldGrid(task->pos2i.x, task->pos2i.y);

			if (task->buildScriptID == BuildScript::CLEAR) {
				context->worldMap->SetPlant(task->pos2i.x, task->pos2i.y, 0, 0);
				context->worldMap->SetRock(task->pos2i.x, task->pos2i.y, 0, false, 0);
				context->worldMap->SetPave(task->pos2i.x, task->pos2i.y, 0);

				Chit* found = chitBag->QueryBuilding(IString(), task->pos2i, 0);
				if (found && (found->GetItem()->IName() != ISC::core)) {
					found->DeRez();
				}
				Remove();
			}
			else if (controller && controller->GetItem()) {
				BuildScript buildScript;
				const BuildData& buildData = buildScript.GetData(task->buildScriptID);

				if (WorkQueue::TaskCanComplete(context->worldMap, context->chitBag, task->pos2i,
					task->buildScriptID,
					controller->GetItem()->wallet))
				{
					// Auto-Clear plants & rock
					Rectangle2I clearBounds;
					clearBounds.Set(task->pos2i.x, task->pos2i.y, task->pos2i.x + buildData.size - 1, task->pos2i.y + buildData.size - 1);
					for (Rectangle2IIterator it(clearBounds); !it.Done(); it.Next()) {
						context->worldMap->SetPlant(it.Pos().x, it.Pos().y, 0, 0);
						context->worldMap->SetRock(it.Pos().x, it.Pos().y, 0, false, 0);
					}

					// Now build.
					if (task->buildScriptID == BuildScript::ICE) {
						context->worldMap->SetRock(task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE);
					}
					else if (task->buildScriptID == BuildScript::PAVE) {
						context->worldMap->SetPave(task->pos2i.x, task->pos2i.y, coreScript->GetPave());
					}
					else {
						// Move the build cost to the building. The gold is held there until the
						// building is destroyed
						GameItem* controllerItem = controller->GetItem();
						Chit* building = chitBag->NewBuilding(task->pos2i, buildData.cStructure, chit->Team());

						building->GetWallet()->Deposit(&controllerItem->wallet, buildData.cost);
						// 'data' property used to transfer in the rotation.
						Quaternion q = Quaternion::MakeYRotation(float(task->data));
						building->SetRotation(q);
					}
					Remove();
				}
				else {
					Clear();
				}
			}
			else {
				// Plan has gone bust:
				Clear();
			}
		}
		break;

		case Task::TASK_PICKUP:
		{
			int chitID = task->data;
			Chit* itemChit = chitBag->GetChit(chitID);
			if (itemChit
				&& (ToWorld2F(itemChit->Position()) - ToWorld2F(chit->Position())).Length() <= PICKUP_RANGE
				&& itemChit->GetItemComponent()->NumItems() == 1)	// doesn't have sub-items / intrinsic
			{
				if (chit->GetItemComponent() && chit->GetItemComponent()->CanAddToInventory()) {
					ItemComponent* ic = itemChit->GetItemComponent();
					itemChit->Remove(ic);
					chit->GetItemComponent()->AddToInventory(ic);
					chitBag->DeleteChit(itemChit);
				}
			}
			Remove();
		}
		break;

		case Task::TASK_USE_BUILDING:
		{
			Chit* building = chitBag->QueryPorch(pos2i);
			if (building) {
				IString buildingName = building->GetItem()->IName();

				if (chit->PlayerControlled()) {
					// Not sure how this happened. But do nothing.
				}
				else {
					UseBuilding(building, buildingName);
					for (int i = 1; i < NUM_BUILDING_USED; ++i) {
						buildingsUsed[i] = buildingsUsed[i - 1];
					}
					buildingsUsed[0] = buildingName;
				}
			}
			Remove();
		}
		break;

		case Task::TASK_FLAG:
		{
			CircuitSim* circuit = context->physicsSims->GetCircuitSim(ToSector(pos2i));
			Chit* porch = context->chitBag->QueryPorch(pos2i);
			Chit* building = context->chitBag->QueryBuilding(IString(), pos2i, 0);
			if (porch && circuit) {
				circuit->TriggerSwitch(pos2i);
			}
			if (building && circuit) {
				circuit->TriggerDetector(pos2i);
			}
			coreScript->RemoveFlag(pos2i);
			Remove();
		}
		break;

		default:
		GLASSERT(0);
		break;

	}
}


void TaskList::SocialPulse(const Vector2F& origin )
{
	const GameItem* gameItem = chit->GetItem();
	if (!gameItem) return;

	if ( !(gameItem->flags & GameItem::AI_USES_BUILDINGS )) {
		return;
	}

	LumosChitBag* chitBag	= chit->Context()->chitBag;
	CChitArray arr;
	
	MOBKeyFilter mobFilter;
	ItemFlagFilter flagFilter( GameItem::AI_USES_BUILDINGS, 0 );
	MultiFilter filter( MultiFilter::MATCH_ALL );
	filter.filters.Push( &mobFilter );
	filter.filters.Push( &flagFilter );

	chitBag->QuerySpatialHash( &arr, origin, 1.5f, 0, &filter );

	// How to handle checking they are all friends? Just run
	// the first one and see if they are friendly.

	if ( arr.Size() < 2 ) return;
	for( int i=1; i<arr.Size(); ++i ) {
		ERelate status = Team::Instance()->GetRelationship( arr[0], arr[i] );
		if ( status == ERelate::ENEMY ) {
			// one bad apple ruins the bunch.
			return;
		}
	}

	// No one to talk to.
	if ( arr.Size() <= 1 ) {
		return;
	}

	// Okay, passed checks. Give social happiness.
//	double social = double(arr.Size()) * 0.05;	// too low - spend avatar time trying to make people happy
	Vector3<double> needs = { 0, 0, 0 };

	for( int i=0; i<arr.Size(); ++i ) {
		const Personality& personality = gameItem->GetPersonality();
		double social = double(arr.Size()) * 0.10;
		if (personality.Introvert()) {
			social *= 0.5;	// introverts get less reward from social interaction (go build stuff)
		}
		needs.X(Needs::FUN) += social;

		arr[i]->GetAIComponent()->GetNeedsMutable()->Add( needs );
		if ( chit->GetRenderComponent() ) {
			chit->GetRenderComponent()->AddDeco( "chat", STD_DECO );
		}
	}
}


void TaskList::UseBuilding(Chit* building, const grinliz::IString& buildingName)
{
	//LumosChitBag* chitBag = chit->Context()->chitBag;
	Vector2I pos2i = ToWorld2I(chit->Position());
	Vector2I sector = ToSector(pos2i);
	CoreScript* coreScript = CoreScript::GetCore(sector);
	Chit* controller = coreScript->ParentChit();
	ItemComponent* ic = building->GetItemComponent();

	if (!chit->GetItem()) return;

	// Working buildings. (Not need based.)
	if (buildingName == ISC::vault) {
		GameItem* vaultItem = building->GetItem();
		GLASSERT(vaultItem);
		GLASSERT(building->GetWallet());
		building->GetWallet()->Deposit(&chit->GetItem()->wallet, chit->GetItem()->wallet);

		// Put everything in the vault.
		ItemComponent* vaultIC = building->GetItemComponent();
		vaultIC->TransferInventory(chit->GetItemComponent(), true, IString());

		// Move gold & crystal to the owner.
		if (controller && controller->GetItemComponent()) {
			controller->GetWallet()->Deposit(&vaultItem->wallet, vaultItem->wallet);
		}
		building->SetTickNeeded();
		return;
	}
	if (buildingName == ISC::distillery) {
		GLASSERT(ic);
		ic->TransferInventory(chit->GetItemComponent(), false, ISC::fruit);
		building->SetTickNeeded();
		return;
	}
	if (buildingName == ISC::bar) {
		int nTransfer = ic->TransferInventory(chit->GetItemComponent(), false, ISC::elixir);
		building->SetTickNeeded();
		if (nTransfer) {
			return;	// only return on transfer. else use the bar!
		}
	}

	if (chit->GetItem()->flags & GameItem::AI_USES_BUILDINGS) {
		// Need based.
		bool functional = false;
		Vector3<double> buildingNeeds = ai::Needs::CalcNeedsFullfilledByBuilding(building, chit, &functional);
		if (buildingNeeds.IsZero()) {
			// doesn't have what is needed, etc.
			return;
		}

		//int nElixir = ic->NumCarriedItems(ISC::elixir);

		if (buildingName == ISC::market) {
			GoShopping(building);
		}
		else if (buildingName == ISC::exchange) {
			GoExchange(building);
		}
		else if (buildingName == ISC::factory) {
			UseFactory(building, int(coreScript->GetTech()));
		}
		else if (buildingName == ISC::bed) {
			// Also heal.
			GameItem* item = chit->GetItem();
			GLASSERT(item);
			item->hp += buildingNeeds.X(Needs::ENERGY) * item->TotalHP();
			if (item->hp > item->TotalHP()) {
				item->hp = item->TotalHP();
			}
		}
		else if (buildingName == ISC::bar) {
			// Apply the needs as is...if there is Elixir.
			const GameItem* elixir = ic->FindItem(ISC::elixir);
			GLASSERT(elixir);	// if !elixir, needs should have been ZERO
			if (elixir) {
				GameItem* item = ic->RemoveFromInventory(elixir);
				delete item;
			}
		}
		if (chit->GetAIComponent()) {
			chit->GetAIComponent()->GetNeedsMutable()->Add(buildingNeeds);
		}
	}
}


void TaskList::GoExchange(Chit* exchange)
{
	GameItem* gameItem = chit->GetItem();
	if (!gameItem) return;

	const Personality& personality = gameItem->GetPersonality();
	const int* crystalValue = ReserveBank::Instance()->CrystalValue();
	bool usedExchange = false;
	ReserveBank* bank = ReserveBank::Instance();

	if (personality.Crafting() == Personality::DISLIKES) {
		// Sell all the crystal
		int value = 0;
		int crystal[NUM_CRYSTAL_TYPES] = { 0 };
		for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
			value += gameItem->wallet.Crystal(type) * crystalValue[type];
			crystal[type] = gameItem->wallet.Crystal(type);
		}
		if (value) {
			// Move money from the bank.
			// Move crystal to the exchange.
			gameItem->wallet.Deposit(&bank->wallet, value);
			exchange->GetWallet()->Deposit(&gameItem->wallet, 0, crystal);
			usedExchange = true;
		}
	}
	else {
		const int nPass = (personality.Crafting() == Personality::LIKES) ? 3 : 1;
		for (int pass = 0; pass < nPass; ++pass) {
			for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
				if (exchange->GetWallet()->Crystal(type) && crystalValue[type] <= gameItem->wallet.Gold()) {
					// Money to bank.
					// Crystal from exchange.
					int crystal[NUM_CRYSTAL_TYPES] = { 0 };
					crystal[type] = 1;
					bank->wallet.Deposit(&gameItem->wallet, crystalValue[type]);
					gameItem->wallet.Deposit(exchange->GetWallet(), 0, crystal);
					usedExchange = true;
				}
			}
		}
	}
	if (usedExchange) {
		RenderComponent* rc = chit->GetRenderComponent();
		if (rc) {
			rc->AddDeco("loot");
		}
	}
}


void TaskList::GoShopping(Chit* market)
{
	// Try to keep this as simple as possible.
	// Still complex. Purchasing stuff can invalidate
	// 'ranged', 'melee', and 'shield', so return
	// if that happens. We can come back later.

	const GameItem* ranged = 0, *melee = 0, *shield = 0;
	Vector2I sector = ToSector(chit->Position());
	CoreScript* cs = CoreScript::GetCore(sector);
	Wallet* salesTax = (cs && cs->ParentChit()->GetItem()) ? &cs->ParentChit()->GetItem()->wallet : 0;

	// Transfer money from reserve to market:
	market->GetWallet()->Deposit(ReserveBank::GetWallet(), 1000);

	MarketAI marketAI( market );

	ItemComponent* thisIC = chit->GetItemComponent();
	if (!thisIC) return;
	GameItem* gameItem = chit->GetItem();
	GLASSERT(gameItem);

	for( int i=1; i<thisIC->NumItems(); ++i ) {
		const GameItem* gi = thisIC->GetItem( i );
		if ( gi && !gi->Intrinsic() ) {
			if ( !ranged && gi->ToRangedWeapon()) {
				ranged = gi;
			}
			else if ( !melee && gi->ToMeleeWeapon() ) {
				melee = gi;
			}
			else if ( !shield && gi->ToShield() ) {
				shield = gi;
			}
		}
	}

	// Basic, critical buys:
	bool boughtStuff = false;
	for( int i=0; i<3; ++i ) {
		const GameItem* purchase = 0;
		if ( i == 0 && !ranged ) purchase = marketAI.HasRanged( gameItem->wallet.Gold(), 1 );
		if ( i == 1 && !shield )
			purchase = marketAI.HasShield( gameItem->wallet.Gold(), 1 );
		if ( i == 2 && !melee )  purchase = marketAI.HasMelee(  gameItem->wallet.Gold(), 1 );
		if ( purchase ) {
			MarketAI::Transact(	purchase,
								thisIC,
								market->GetItemComponent(),
								salesTax,
								true );
			boughtStuff = true;
		}
	}

	if (!boughtStuff) {
		// Sell the extras.
		const GameItem* itemToSell = 0;
		while ((itemToSell = thisIC->ItemToSell()) != 0) {
			//int value = itemToSell->GetValue();
			int sold = MarketAI::Transact(itemToSell,
				market->GetItemComponent(),	// buyer
				thisIC,		// seller
				salesTax,					// more of a transaction bonus...
				true);

			if (sold)
				boughtStuff = true;
			else
				break;
		}
	}

	if (!boughtStuff) {
		// Upgrades! Don't set ranged, shield, etc. because we don't want a critical followed 
		// by a non-critical. Also, personality probably should factor in to purchasing decisions.
		for (int i = 0; i < 3; ++i) {
			const GameItem* purchase = 0;
			if (i == 0 && ranged) purchase = marketAI.HasRanged(gameItem->wallet.Gold(), ranged->GetValue() * 2);
			if (i == 1 && shield) purchase = marketAI.HasShield(gameItem->wallet.Gold(), shield->GetValue() * 2);
			if (i == 2 && melee)  purchase = marketAI.HasMelee(gameItem->wallet.Gold(), melee->GetValue() * 2);
			if (purchase) {

				MarketAI::Transact(purchase,
					thisIC,
					market->GetItemComponent(),
					salesTax,
					true);
			}
		}
	}
	ReserveBank::GetWallet()->Deposit(market->GetWallet(), *(market->GetWallet()));
}


bool TaskList::UseFactory( Chit* factory, int tech )
{
	ItemComponent* thisIC = chit->GetItemComponent();
	if (!thisIC) return false;
	if (!thisIC->CanAddToInventory()) return false;
	GameItem* gameItem = chit->GetItem();
	GLASSERT(gameItem);

	RangedWeapon* ranged = thisIC->GetRangedWeapon(0);
	MeleeWeapon* melee = thisIC->GetMeleeWeapon();
	Shield* shield = thisIC->GetShield();

	int itemType = 0;
	Random& random = chit->random;

	if ( !ranged )		itemType = ForgeScript::GUN;
	else if ( !shield ) itemType = ForgeScript::SHIELD;
	else if ( !melee )	itemType = ForgeScript::RING;
	else {
		itemType = random.Rand( ForgeScript::NUM_ITEM_TYPES );
	}

	int seed = chit->ID() ^ gameItem->Traits().Experience();
	int level = gameItem->Traits().Level();
	TransactAmt cost;
	int partsMask = 0xffffffff;
	int team = Team::Group(chit->Team());
	int subItem = -1;
	//const char* altRes = "";

	// Special rules.
	if (itemType == ForgeScript::RING) {
		// Only trolls and gobs use the blades.
		if (team == TEAM_TROLL || team == TEAM_GOB) 
			partsMask &= (~WeaponGen::RING_TRIAD);
		else 
			partsMask &= (~WeaponGen::RING_BLADE);
	}

	// FIXME: the parts mask (0xff) is set for denizen domains.
	GameItem* item = ForgeScript::DoForge(itemType, subItem, 
										  gameItem->wallet, &cost, 
										  0xffffffff, 0xffffffff, 
										  tech, level, seed, chit->Team());
	if (item) {
		if (team == TEAM_KAMAKIRI && item->IName() == ISC::beamgun) {
			item->SetResource("kamabeamgun");
		}

		item->wallet.Deposit(&gameItem->wallet, cost);
		thisIC->AddToInventory(item);
		thisIC->AddCraftXP(cost.NumCrystals());
		gameItem->historyDB.Increment("Crafted");

		Vector2I sector = ToSector(chit->Position());
		GLOUTPUT(("'%s' forged the item '%s' at sector=%x,%x\n",
			gameItem->BestName(),
			item->BestName(),
			sector.x, sector.y));

		item->SetSignificant(chit->Context()->chitBag->GetNewsHistory(), 
							 ToWorld2F(chit->Position()),
							 NewsEvent::FORGED, NewsEvent::UN_FORGED, chit);
		return true;
	}
	return false;
}


bool TaskList::UsingBuilding() const
{
	if (taskList.Size() >= 2) {
		if (taskList[0].action == Task::TASK_STAND
			&& taskList[1].action == Task::TASK_USE_BUILDING)
		{
			return true;
		}
	}
	return false;
}
