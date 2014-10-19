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

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/rendercomponent.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

#include "../script/corescript.h"
#include "../script/buildscript.h"
#include "../script/forgescript.h"
#include "../script/procedural.h"
#include "../script/plantscript.h"
#include "../script/evalbuildingscript.h"

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
	XARC_SER( xs, lastBuildingUsed );
	XARC_SER_CARRAY( xs, taskList );
	socialTicker.Serialize( xs, "socialTicker" );
	XarcClose( xs );
}


bool TaskList::DoStanding( const ComponentSet& thisComp, int time )
{
	if ( taskList.Empty() ) return false;
	if ( Standing() ) {
		taskList[0].timer -= time;
		if ( taskList[0].timer <= 0 ) {
			Remove();
		}
		int n = socialTicker.Delta( time );
		for( int i=0; i<n; ++i ) {
			SocialPulse( thisComp, thisComp.spatial->GetPosition2D() );
		}
		return true;
	}
	return false;
}


void TaskList::DoTasks(Chit* chit, U32 delta)
{
	if (taskList.Empty()) return;

	ComponentSet thisComp(chit, Chit::MOVE_BIT | Chit::SPATIAL_BIT | Chit::AI_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE);
	PathMoveComponent* pmc = GET_SUB_COMPONENT(chit, MoveComponent, PathMoveComponent);

	if (!pmc || !thisComp.okay) {
		Clear();
		return;
	}

	LumosChitBag* chitBag = chit->Context()->chitBag;
	Task* task = &taskList[0];
	Vector2I pos2i = thisComp.spatial->GetPosition2DI();
	Vector2F taskPos2 = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
	Vector3F taskPos3 = { taskPos2.x, 0, taskPos2.y };
	Vector2I sector = ToSector(pos2i);
	Rectangle2I innerBounds = InnerSectorBounds(sector);
	CoreScript* coreScript = CoreScript::GetCore(sector);
	Chit* controller = coreScript ? coreScript->ParentChit() : 0;

	switch (task->action) {
		case Task::TASK_MOVE:
		if (task->timer == 0) {
			// Need to start the move.
			Vector2F dest = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
			thisComp.ai->Move(dest, false);
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
			thisComp.ai->Stand();
			DoStanding(thisComp, delta);

			if (taskList.Size() >= 2) {
				int action = taskList[1].action;
				if (action == Task::TASK_BUILD) {
					Vector3F pos = ToWorld3F(taskList[1].pos2i);
					pos.y = 0.5f;
					context->engine->particleSystem->EmitPD(ISC::construction, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
				else if (action == Task::TASK_USE_BUILDING) {
					Vector3F pos = thisComp.spatial->GetPosition();
					context->engine->particleSystem->EmitPD(ISC::useBuilding, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
				else if (action == Task::TASK_REPAIR_BUILDING) {
					Vector3F pos = thisComp.spatial->GetPosition();
					context->engine->particleSystem->EmitPD(ISC::repair, pos, V3F_UP, 30);	// FIXME: standard delta constant					
				}
			}
		}
		break;

		case Task::TASK_REPAIR_BUILDING:
		{
			Chit* building = chitBag->GetChit(task->data);
			if (building) {
				MapSpatialComponent* msc = building->GetSpatialComponent()->ToMapSpatialComponent();
				if (msc) {
					Rectangle2I b = msc->Bounds();
					b.Outset(1);
					if (b.Contains(pos2i)) {
						ComponentSet comp(building, ComponentSet::IS_ALIVE | Chit::ITEM_BIT);
						if (comp.okay) {
							comp.item->hp = double(comp.item->TotalHP());
							RenderComponent* rc = building->GetRenderComponent();
							if (rc) {
								rc->AddDeco("repair", STD_DECO);
							}
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

			const WorldGrid& wg = context->worldMap->GetWorldGrid(task->pos2i.x, task->pos2i.y);

			if (task->buildScriptID == BuildScript::CLEAR) {
				context->worldMap->SetPlant(task->pos2i.x, task->pos2i.y, 0, 0);
				context->worldMap->SetRock(task->pos2i.x, task->pos2i.y, 0, false, 0);
				context->worldMap->SetPave(task->pos2i.x, task->pos2i.y, 0);

				context->worldMap->SetCircuit(task->pos2i.x, task->pos2i.y, 0);
				context->circuitSim->EtchLines(innerBounds);

				Chit* found = chitBag->QueryRemovable(task->pos2i);
				if (found) {
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

					// Now build. The Rock/Pave/Building may coexist with a plant for a frame,
					// but the plant will be de-rezzed at the next tick.
					if (task->buildScriptID == BuildScript::ICE) {
						context->worldMap->SetRock(task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE);
					}
					else if (task->buildScriptID == BuildScript::PAVE) {
						context->worldMap->SetPave(task->pos2i.x, task->pos2i.y, coreScript->GetPave());
					}
					else if (buildData.circuit) {
						context->worldMap->SetCircuit(task->pos2i.x, task->pos2i.y, buildData.circuit);
						context->circuitSim->EtchLines(innerBounds);
					}
					else {
						// Move the build cost to the building. The gold is held there until the
						// building is destroyed
						GameItem* controllerItem = controller->GetItem();
						Chit* building = chitBag->NewBuilding(task->pos2i, buildData.cStructure, chit->Team());

						building->GetWallet()->Deposit(&controllerItem->wallet, buildData.cost);
						// 'data' property used to transfer in the rotation.
						building->GetSpatialComponent()->SetYRotation(float(task->data));
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
				&& itemChit->GetSpatialComponent()
				&& (itemChit->GetSpatialComponent()->GetPosition2D() - thisComp.spatial->GetPosition2D()).Length() <= PICKUP_RANGE
				&& itemChit->GetItemComponent()->NumItems() == 1)	// doesn't have sub-items / intrinsic
			{
				if (thisComp.itemComponent->CanAddToInventory()) {
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
			Chit* building = chitBag->QueryPorch(pos2i, 0);
			if (building) {
				IString buildingName = building->GetItem()->IName();

				if (chit->PlayerControlled()) {
					// Not sure how this happened. But do nothing.
				}
				else {
					UseBuilding(thisComp, building, buildingName);
					lastBuildingUsed = buildingName;
				}
			}
			Remove();
		}
		break;

		case Task::TASK_FLAG:
		{
			const WorldGrid wg = context->worldMap->GetWorldGrid(pos2i);
			if (wg.Circuit() == CIRCUIT_SWITCH) {
				context->circuitSim->TriggerSwitch(pos2i);
			}
			else if (wg.Circuit() == CIRCUIT_DETECT_ENEMY) {
				context->circuitSim->TriggerDetector(pos2i);
			}
			// FIXME: what about flags in other domains?
			coreScript->RemoveFlag(pos2i);
			Remove();
		}
		break;

		default:
		GLASSERT(0);
		break;

	}
}


void TaskList::SocialPulse( const ComponentSet& thisComp, const Vector2F& origin )
{
	if ( !(thisComp.item->flags & GameItem::AI_USES_BUILDINGS )) {
		return;
	}

	LumosChitBag* chitBag	= thisComp.chit->Context()->chitBag;
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
		int status = Team::GetRelationship( arr[0], arr[i] );
		if ( status == RELATE_ENEMY ) {
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
	for( int i=0; i<arr.Size(); ++i ) {
		const Personality& personality = thisComp.item->GetPersonality();
		double social = double(arr.Size()) * 0.10;
		if (personality.Introvert()) {
			social *= 0.5;	// introverts get less reward from social interaction (go build stuff)
		}

		arr[i]->GetAIComponent()->GetNeedsMutable()->Add( ai::Needs::FUN, social );
		if ( thisComp.chit->GetRenderComponent() ) {
			thisComp.chit->GetRenderComponent()->AddDeco( "chat", STD_DECO );
		}
	}
}


void TaskList::UseBuilding( const ComponentSet& thisComp, Chit* building, const grinliz::IString& buildingName )
{
	LumosChitBag* chitBag	= thisComp.chit->Context()->chitBag;
	Vector2I pos2i			= thisComp.spatial->GetPosition2DI();
	Vector2I sector			= ToSector( pos2i );
	CoreScript* coreScript  = CoreScript::GetCore(sector);
	Chit* controller		= coreScript->ParentChit();
	ItemComponent* ic		= building->GetItemComponent();

	// Workers:
	if ( buildingName == ISC::vault ) {
		GameItem* vaultItem = building->GetItem();
		GLASSERT( vaultItem );
		GLASSERT(building->GetWallet());
		building->GetWallet()->Deposit(&thisComp.item->wallet, thisComp.item->wallet);

		// Put everything in the vault.
		ItemComponent* vaultIC = building->GetItemComponent();
		vaultIC->TransferInventory( thisComp.itemComponent, true, IString() );

		// Move gold & crystal to the owner.
		if ( controller && controller->GetItemComponent() ) {
			controller->GetWallet()->Deposit(&vaultItem->wallet, vaultItem->wallet);
		}
		building->SetTickNeeded();
		return;
	}
	if ( buildingName == ISC::distillery ) {
		GLASSERT( ic );
		ic->TransferInventory( thisComp.itemComponent, false, ISC::fruit );
		building->SetTickNeeded();
		return;
	}
	if (buildingName == ISC::bar) {
		int nTransfer = ic->TransferInventory( thisComp.itemComponent, false, ISC::elixir );
		building->SetTickNeeded();
		if (nTransfer) {
			return;	// only return on transfer. else use the bar!
		}
	}

	if ( thisComp.item->flags & GameItem::AI_USES_BUILDINGS ) {
		BuildScript buildScript;
		const BuildData* bd = buildScript.GetDataFromStructure( buildingName, 0 );
		GLASSERT( bd );
		ai::Needs supply = bd->needs;
		int nElixir = ic->NumCarriedItems(ISC::elixir);

		// Food based buildings don't work if there is no elixir.
		if ( supply.Value(Needs::FOOD) && nElixir == 0 ) {
			supply.SetZero();
		}

		if ( buildingName == ISC::market ) {
			GoShopping( thisComp, building );
		}
		else if (buildingName == ISC::exchange) {
			GoExchange(thisComp, building);
		}
		else if ( buildingName == ISC::factory ) {
			bool used = UseFactory( thisComp, building, int(coreScript->GetTech()) );
			if ( !used ) supply.SetZero();
		}
		else if ( buildingName == ISC::bed ) {
			// Apply the needs as is.
		}
		else if ( buildingName == ISC::bar ) {
			// Apply the needs as is...if there is Elixir.
		}
		else {
			GLASSERT( 0 );
		}
		if ( supply.Value(Needs::FOOD) > 0 ) {
			GLASSERT( supply.Value(Needs::FOOD) == 1 );	// else probably not what intended.
			GLASSERT( nElixir > 0 );

			const GameItem* elixir = ic->FindItem(ISC::elixir);
			GLASSERT(elixir);
			if (elixir) {
				GameItem* item = ic->RemoveFromInventory(elixir);
				delete item;
			}
		}
		// Social attracts, but is never applied. (That is what the SocialPulse is for.)
		//supply.Set(Needs::SOCIAL, 0);

		double scale = 1.0;

		EvalBuildingScript* evalScript = static_cast<EvalBuildingScript*>(building->GetComponent("EvalBuildingScript"));
		if (evalScript) {
			double industry = building->GetItem()->GetBuildingIndustrial();
			double score = evalScript->EvalIndustrial(true);
			double dot = score * industry;
			scale = 0.55 + 0.45 * dot;

			if (!evalScript->Reachable()) {
				scale = 0;
			}
		}
		thisComp.ai->GetNeedsMutable()->Add( supply, scale );

		double heal = (supply.Value(Needs::ENERGY) + supply.Value(Needs::FOOD)) * scale;
		heal = Clamp( heal, 0.0, 1.0 );

		thisComp.item->hp += double(thisComp.item->TotalHP()) * heal;
		thisComp.item->hp = Min( thisComp.item->hp, double(thisComp.item->TotalHP()) );
	}
}


void TaskList::GoExchange(const ComponentSet& thisComp, Chit* exchange)
{
	const Personality& personality = thisComp.item->GetPersonality();
	const int* crystalValue = ReserveBank::Instance()->CrystalValue();
	bool usedExchange = false;
	ReserveBank* bank = ReserveBank::Instance();

	if (personality.Crafting() == Personality::DISLIKES) {
		// Sell all the crystal
		int value = 0;
		int crystal[NUM_CRYSTAL_TYPES] = { 0 };
		for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
			value += thisComp.item->wallet.Crystal(type) * crystalValue[type];
			crystal[type] = thisComp.item->wallet.Crystal(type);
		}
		if (value) {
			// Move money from the bank.
			// Move crystal to the exchange.
			thisComp.item->wallet.Deposit(&bank->wallet, value);
			exchange->GetWallet()->Deposit(&thisComp.item->wallet, 0, crystal);
			usedExchange = true;
		}
	}
	else {
		int willSpend = thisComp.item->wallet.Gold() / 5;
		if (personality.Crafting() == Personality::LIKES) {
			willSpend = thisComp.item->wallet.Gold() / 2;
		}

		for (int pass = 0; pass < 3; ++pass) {
			for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
				if (exchange->GetWallet()->Crystal(type) && crystalValue[type] <= willSpend) {
					// Money to bank.
					// Crystal from exchange.
					int crystal[NUM_CRYSTAL_TYPES] = { 0 };
					crystal[type] = 1;
					bank->wallet.Deposit(&thisComp.item->wallet, crystalValue[type]);
					thisComp.item->wallet.Deposit(exchange->GetWallet(), 0, crystal);
					willSpend -= crystalValue[type];
					usedExchange = true;
				}
			}
		}
	}
	if (usedExchange) {
		RenderComponent* rc = thisComp.chit->GetRenderComponent();
		if (rc) {
			rc->AddDeco("loot");
		}
	}
}


void TaskList::GoShopping(const ComponentSet& thisComp, Chit* market)
{
	// Try to keep this as simple as possible.
	// Still complex. Purchasing stuff can invalidate
	// 'ranged', 'melee', and 'shield', so return
	// if that happens. We can come back later.

	const GameItem* ranged = 0, *melee = 0, *shield = 0;
	Vector2I sector = ToSector( thisComp.spatial->GetPosition2DI() );
	CoreScript* cs = CoreScript::GetCore(sector);
	Wallet* salesTax = (cs && cs->ParentChit()->GetItem()) ? &cs->ParentChit()->GetItem()->wallet : 0;

	// Transfer money from reserve to market:
	market->GetWallet()->Deposit(ReserveBank::GetWallet(), 1000);

	MarketAI marketAI( market );

	// Should be sorted, but just in case:
//	thisComp.itemComponent->SortInventory();
//	market->GetItemComponent()->SortInventory();

	for( int i=1; i<thisComp.itemComponent->NumItems(); ++i ) {
		const GameItem* gi = thisComp.itemComponent->GetItem( i );
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
		if ( i == 0 && !ranged ) purchase = marketAI.HasRanged( thisComp.item->wallet.Gold(), 1 );
		if ( i == 1 && !shield )
			purchase = marketAI.HasShield( thisComp.item->wallet.Gold(), 1 );
		if ( i == 2 && !melee )  purchase = marketAI.HasMelee(  thisComp.item->wallet.Gold(), 1 );
		if ( purchase ) {
			MarketAI::Transact(	purchase,
								thisComp.itemComponent,
								market->GetItemComponent(),
								salesTax,
								true );
			boughtStuff = true;
		}
	}

	if (!boughtStuff) {
		// Sell the extras.
		const GameItem* itemToSell = 0;
		while ((itemToSell = thisComp.itemComponent->ItemToSell()) != 0) {
			int value = itemToSell->GetValue();
			int sold = MarketAI::Transact(itemToSell,
				market->GetItemComponent(),	// buyer
				thisComp.itemComponent,		// seller
				0,							// no sales tax when selling to the market.
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
			if (i == 0 && ranged) purchase = marketAI.HasRanged(thisComp.item->wallet.Gold(), ranged->GetValue() * 2);
			if (i == 1 && shield) purchase = marketAI.HasShield(thisComp.item->wallet.Gold(), shield->GetValue() * 2);
			if (i == 2 && melee)  purchase = marketAI.HasMelee(thisComp.item->wallet.Gold(), melee->GetValue() * 2);
			if (purchase) {

				MarketAI::Transact(purchase,
					thisComp.itemComponent,
					market->GetItemComponent(),
					salesTax,
					true);
			}
		}
	}
	ReserveBank::GetWallet()->Deposit(market->GetWallet(), *(market->GetWallet()));
}


bool TaskList::UseFactory( const ComponentSet& thisComp, Chit* factory, int tech )
{
	if ( !thisComp.itemComponent->CanAddToInventory() ) {
		return false;
	}

	RangedWeapon* ranged = thisComp.itemComponent->GetRangedWeapon(0);
	MeleeWeapon* melee = thisComp.itemComponent->GetMeleeWeapon();
	Shield* shield = thisComp.itemComponent->GetShield();

	int itemType = 0;
	Random& random = thisComp.chit->random;

	if ( !ranged )		itemType = ForgeScript::GUN;
	else if ( !shield ) itemType = ForgeScript::SHIELD;
	else if ( !melee )	itemType = ForgeScript::SHIELD;
	else {
		itemType = random.Rand( ForgeScript::NUM_ITEM_TYPES );
	}

	int seed = thisComp.chit->ID() ^ thisComp.item->Traits().Experience();
	int level = thisComp.item->Traits().Level();
	TransactAmt cost;

	// FIXME: the parts mask (0xff) is set for denizen domains.
	GameItem* item = ForgeScript::DoForge(itemType, thisComp.item->wallet, &cost, 0xffffffff, 0xffffffff, tech, level, seed);
	if (item) {
		item->wallet.Deposit(&thisComp.item->wallet, cost);
		thisComp.itemComponent->AddToInventory(item);
		thisComp.itemComponent->AddCraftXP(cost.NumCrystals());
		thisComp.item->historyDB.Increment("Crafted");

		Vector2I sector = thisComp.spatial->GetPosition2DI();
		GLOUTPUT(("'%s' forged the item '%s' at sector=%x,%x\n",
			thisComp.item->BestName(),
			item->BestName(),
			sector.x, sector.y));

		item->SetSignificant(thisComp.chit->Context()->chitBag->GetNewsHistory(), 
							 thisComp.spatial->GetPosition2D(),
							 NewsEvent::FORGED, NewsEvent::UN_FORGED, thisComp.chit);
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
