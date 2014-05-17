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

Task Task::RemoveTask( const grinliz::Vector2I& pos2i, int taskID )
{
	return Task::BuildTask( pos2i, BuildScript::CLEAR, taskID );
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
	XARC_SER( xs, taskID );
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


void TaskList::DoTasks( Chit* chit, WorkQueue* workQueue, U32 delta )
{
	if ( taskList.Empty() ) return;

	ComponentSet thisComp( chit, Chit::MOVE_BIT | Chit::SPATIAL_BIT | Chit::AI_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
	PathMoveComponent* pmc = GET_SUB_COMPONENT( chit, MoveComponent, PathMoveComponent );

	if ( !pmc || !thisComp.okay ) {
		Clear();
		return;
	}

	LumosChitBag* chitBag = chit->GetLumosChitBag();
	Task* task			= &taskList[0];
	Vector2I pos2i		= thisComp.spatial->GetPosition2DI();
	Vector2F taskPos2	= { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
	Vector3F taskPos3	= { taskPos2.x, 0, taskPos2.y };
	Vector2I sector		= ToSector( pos2i );
	CoreScript* coreScript = CoreScript::GetCore(sector);
	Chit* controller = coreScript ? coreScript->ParentChit() : 0;

	// If this is a task associated with a work item, make
	// sure that work item still exists.
	if ( task->taskID ) {
		const WorkQueue::QueueItem* queueItem = 0;
		if ( workQueue ) {
			queueItem = workQueue->GetJobByTaskID( task->taskID );
		}
		if ( !queueItem ) {
			// This task it attached to a work item that expired.
			Clear();
			return;
		}
	}

	switch ( task->action ) {
	case Task::TASK_MOVE:
		if (task->timer == 0) {
			// Need to start the move.
			Vector2F dest = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
			thisComp.ai->Move( dest, false );
			task->timer = delta;
		}
		else if ( pmc->Stopped() ) {
			if ( pos2i == task->pos2i ) {
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
		if ( pmc->Stopped() ) {
			thisComp.ai->Stand();
			DoStanding( thisComp, delta );

			if ( taskList.Size() >= 2 ) {
				int action = taskList[1].action;
				if ( action == Task::TASK_BUILD ) {
					Vector3F pos = ToWorld3F( taskList[1].pos2i );
					pos.y = 0.5f;
					engine->particleSystem->EmitPD( "construction", pos, V3F_UP, 30 );	// FIXME: standard delta constant					
				}
				else if ( action == Task::TASK_USE_BUILDING ) {
					Vector3F pos = thisComp.spatial->GetPosition();
					engine->particleSystem->EmitPD( "useBuilding", pos, V3F_UP, 30 );	// FIXME: standard delta constant					
				}
				else if (action == Task::TASK_REPAIR_BUILDING) {
					Vector3F pos = thisComp.spatial->GetPosition();
					engine->particleSystem->EmitPD("repair", pos, V3F_UP, 30);	// FIXME: standard delta constant					
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
							comp.item->hp = comp.item->TotalHPF();
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

			const WorldGrid& wg = worldMap->GetWorldGrid( task->pos2i.x, task->pos2i.y );

			if ( task->buildScriptID == BuildScript::CLEAR ) {
				if ( wg.RockHeight() ) {
					DamageDesc dd( 10000, 0 );	// FIXME need constant
					Vector3I voxel = { task->pos2i.x, 0, task->pos2i.y };
					worldMap->VoxelHit( voxel, dd );
				}
				else {
					Chit* found = chitBag->QueryRemovable( task->pos2i, false );
					if ( found ) {
						found->DeRez();
					}
				}
				if ( wg.Pave() ) {
					worldMap->SetPave( task->pos2i.x, task->pos2i.y, 0 );
				}
				Remove();
			}
			else if ( controller && controller->GetItem() ) {
				BuildScript buildScript;
				const BuildData& buildData	= buildScript.GetData( task->buildScriptID );

				if ( WorkQueue::TaskCanComplete( worldMap, chitBag, task->pos2i,
					task->buildScriptID,
					controller->GetItem()->wallet ))
				{
					// Auto-Clear plants.
					Rectangle2F clearBounds;
					clearBounds.Set( (float)task->pos2i.x, (float)task->pos2i.y, (float)(task->pos2i.x + buildData.size), (float)(task->pos2i.y + buildData.size));
					CChitArray plants;
					PlantFilter plantFilter;
					chitBag->QuerySpatialHash( &plants, clearBounds, 0, &plantFilter );
					for( int k=0; k<plants.Size(); ++k ) {
						plants[k]->DeRez();
						// Some hackery: we can't build until the plant isn't there,
						// but the component won't get deleted until later. Remove
						// the MapSpatialComponent to detatch it from map.
						MapSpatialComponent* msc = GET_SUB_COMPONENT( plants[k], SpatialComponent, MapSpatialComponent );
						GLASSERT( msc );
						if ( msc ) {
							plants[k]->Remove( msc );
							delete msc;
						}
					}

					// Now build. The Rock/Pave/Building may coexist with a plant for a frame,
					// but the plant will be de-rezzed at the next tick.
					if ( task->buildScriptID == BuildScript::ICE ) {
						worldMap->SetRock( task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE );
					}
					else if ( task->buildScriptID == BuildScript::PAVE ) {
						worldMap->SetPave( task->pos2i.x, task->pos2i.y, chit->PrimaryTeam()%3+1 );
					}
					else {
						// Move the build cost to the building. The gold is held there until the
						// building is destroyed
						GameItem* controllerItem = controller->GetItem();
						Chit* building = chitBag->NewBuilding( task->pos2i, buildData.cStructure, chit->PrimaryTeam() );
						Transfer(&building->GetItem()->wallet, &controllerItem->wallet, buildData.cost);
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

			// No matter if successful or not, we need to clear the work item.
			// The work is done or can't be done.
			if ( workQueue ) {
				workQueue->Remove( pos2i );
			}
		}
		break;

	case Task::TASK_PICKUP:
		{
			int chitID = task->data;
			Chit* itemChit = chitBag->GetChit( chitID );
			if (    itemChit 
				 && itemChit->GetSpatialComponent()
				 && (itemChit->GetSpatialComponent()->GetPosition2D() - thisComp.spatial->GetPosition2D()).Length() <= PICKUP_RANGE
				 && itemChit->GetItemComponent()->NumItems() == 1 )	// doesn't have sub-items / intrinsic
			{
				if ( thisComp.itemComponent->CanAddToInventory() ) {
					ItemComponent* ic = itemChit->GetItemComponent();
					itemChit->Remove( ic );
					chit->GetItemComponent()->AddToInventory( ic );
					chitBag->DeleteChit( itemChit );
				}
			}
			Remove();
		}
		break;

	case Task::TASK_USE_BUILDING:
		{
			Chit* building	= chitBag->QueryPorch( pos2i, 0 );
			if ( building ) {
				IString buildingName = building->GetItem()->IName();

				if ( chit->PlayerControlled() ) {
					// Not sure how this happened. But do nothing.
				}
				else {
					UseBuilding( thisComp, building, buildingName );
					lastBuildingUsed = buildingName;
				}
			}
			Remove();
		}
		break;

	default:
		GLASSERT( 0 );
		break;

	}
}


void TaskList::SocialPulse( const ComponentSet& thisComp, const Vector2F& origin )
{
	if ( !(thisComp.item->flags & GameItem::AI_USES_BUILDINGS )) {
		return;
	}

	LumosChitBag* chitBag	= thisComp.chit->GetLumosChitBag();
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
		int status = arr[0]->GetAIComponent()->GetTeamStatus( arr[i] );
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
	double social = double(arr.Size()) * 0.10;
	for( int i=0; i<arr.Size(); ++i ) {
		arr[i]->GetAIComponent()->GetNeedsMutable()->Add( ai::Needs::SOCIAL, social );
		if ( thisComp.chit->GetRenderComponent() ) {
			thisComp.chit->GetRenderComponent()->AddDeco( "chat", STD_DECO );
		}
	}
}


void TaskList::UseBuilding( const ComponentSet& thisComp, Chit* building, const grinliz::IString& buildingName )
{
	LumosChitBag* chitBag	= thisComp.chit->GetLumosChitBag();
	Vector2I pos2i			= thisComp.spatial->GetPosition2DI();
	Vector2I sector			= ToSector( pos2i );
	CoreScript* coreScript  = CoreScript::GetCore(sector);
	Chit* controller		= coreScript->ParentChit();

	// Workers:
	if ( buildingName == IStringConst::vault ) {
		GameItem* vaultItem = building->GetItem();
		GLASSERT( vaultItem );
		Transfer(&vaultItem->wallet, &thisComp.item->wallet, thisComp.item->wallet);

		// Put everything in the vault.
		ItemComponent* vaultIC = building->GetItemComponent();
		vaultIC->AddSubInventory( thisComp.itemComponent, true, IString() );

		// Move gold & crystal to the owner.
		if ( controller && controller->GetItemComponent() ) {
			Transfer(&controller->GetItem()->wallet, &vaultItem->wallet, vaultItem->wallet);
		}
		building->SetTickNeeded();
		return;
	}
	if ( buildingName == IStringConst::distillery ) {
		ItemComponent* ic = building->GetItemComponent();
		GLASSERT( ic );
		ic->AddSubInventory( thisComp.itemComponent, false, IStringConst::fruit );
		building->SetTickNeeded();
		return;
	}

	if ( thisComp.item->flags & GameItem::AI_USES_BUILDINGS ) {
		BuildScript buildScript;
		const BuildData* bd = buildScript.GetDataFromStructure( buildingName );
		GLASSERT( bd );
		ai::Needs supply = bd->needs;

		// Food based buildings don't work if there is no elixir.
		if ( supply.Value(Needs::FOOD) && coreScript->nElixir == 0 ) {
			supply.SetZero();
		}

		if ( buildingName == IStringConst::market ) {
			GoShopping( thisComp, building );
		}
		else if (buildingName == IStringConst::exchange) {
			GoExchange(thisComp, building);
		}
		else if ( buildingName == IStringConst::factory ) {
			bool used = UseFactory( thisComp, building, int(coreScript->GetTech()) );
			if ( !used ) supply.SetZero();
		}
		else if ( buildingName == IStringConst::bed ) {
			// Apply the needs as is.
		}
		else if ( buildingName == IStringConst::bar ) {
			// Apply the needs as is...if there is Elixir.
		}
		else {
			GLASSERT( 0 );
		}
		if ( supply.Value(Needs::FOOD) > 0 ) {
			GLASSERT( supply.Value(Needs::FOOD) == 1 );	// else probably not what intended.
			GLASSERT( coreScript->nElixir > 0 );
			coreScript->nElixir -= 1;
		}
		// Social attracts, but is never applied. (That is what the SocialPulse is for.)
		supply.Set(Needs::SOCIAL, 0);

		double scale = 1.0;

		EvalBuildingScript* evalScript = static_cast<EvalBuildingScript*>(building->GetScript("EvalBuildingScript"));
		if (evalScript) {
			double industry = building->GetItem()->GetBuildingIndustrial(false);
			double score = evalScript->EvalIndustrial(true);
			double dot = score * industry;
			scale = 0.55 + 0.45 * dot;

			if (!evalScript->Reachable()) {
				scale = 0;
			}
		}
		thisComp.ai->GetNeedsMutable()->Add( supply, scale );

		float heal = float(supply.Value(Needs::ENERGY) + supply.Value(Needs::FOOD)) * float(scale);
		heal = Clamp( heal, 0.f, 1.f );

		thisComp.item->hp += thisComp.item->TotalHPF() * heal;
		thisComp.item->hp = Min( thisComp.item->hp, thisComp.item->TotalHPF() );
	}
}


void TaskList::GoExchange(const ComponentSet& thisComp, Chit* exchange)
{
	const Personality& personality = thisComp.item->GetPersonality();
	const int* crystalValue = ReserveBank::Instance()->CrystalValue();
	bool usedExchange = false;

	if (personality.Crafting() == Personality::DISLIKES) {
		// Sell all the crystal
		for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
			int n = thisComp.item->wallet.crystal[type];
			if (n) {
				// Gold from bank.
				Wallet c;
				c.crystal[type] = n;

				if (CanTransfer(&thisComp.item->wallet, &ReserveBank::Instance()->bank, n*crystalValue[type])) {
					Transfer(&thisComp.item->wallet, &ReserveBank::Instance()->bank, n*crystalValue[type]);
					Transfer(&ReserveBank::Instance()->bank, &thisComp.item->wallet, c);
					usedExchange = true;
				}
			}
		}
	}
	else {
		int willSpend = thisComp.item->wallet.gold / 5;
		if (personality.Crafting() == Personality::LIKES) {
			willSpend = thisComp.item->wallet.gold / 2;
		}

		for (int type = 0; type < NUM_CRYSTAL_TYPES; ++type) {
			int nWant = NUM_CRYSTAL_TYPES - type - thisComp.item->wallet.crystal[type];
	
			while (nWant) {
				int cost = crystalValue[type];
				if (cost < willSpend) {
					Wallet c;
					c.crystal[type] = 1;

					if (CanTransfer(&thisComp.item->wallet, &exchange->GetItem()->wallet, c)) {
						Transfer(&ReserveBank::Instance()->bank, &thisComp.item->wallet, cost);
						Transfer(&thisComp.item->wallet, &exchange->GetItem()->wallet, c);
						--nWant;
						willSpend -= cost;
						usedExchange = true;
					}
					else {
						break;
					}
				}
				else {
					break;
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

	GameItem* ranged = 0, *melee = 0, *shield = 0;
	Vector2I sector = ToSector( thisComp.spatial->GetPosition2DI() );
	CoreScript* cs = CoreScript::GetCore(sector);
	Wallet* salesTax = (cs && cs->ParentChit()->GetItem()) ? &cs->ParentChit()->GetItem()->wallet : 0;

	MarketAI marketAI( market );

	// Should be sorted, but just in case:
	thisComp.itemComponent->SortInventory();
	market->GetItemComponent()->SortInventory();

	for( int i=1; i<thisComp.itemComponent->NumItems(); ++i ) {
		GameItem* gi = thisComp.itemComponent->GetItem( i );
		if ( gi && !gi->Intrinsic() ) {
			if ( !ranged && (gi->flags & GameItem::RANGED_WEAPON)) {
				ranged = gi;
			}
			else if ( !melee && !(gi->flags & GameItem::RANGED_WEAPON) && (gi->flags & GameItem::MELEE_WEAPON) ) {
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
		if ( i == 0 && !ranged ) purchase = marketAI.HasRanged( thisComp.item->wallet.gold, 1 );
		if ( i == 1 && !shield )
			purchase = marketAI.HasShield( thisComp.item->wallet.gold, 1 );
		if ( i == 2 && !melee )  purchase = marketAI.HasMelee(  thisComp.item->wallet.gold, 1 );
		if ( purchase ) {
			MarketAI::Transact(	purchase,
								thisComp.itemComponent,
								market->GetItemComponent(),
								salesTax,
								true );
			boughtStuff = true;
		}
	}
	if (boughtStuff) return;

	// Sell the extras.
	int itemToSell = 0;
	while ((itemToSell = thisComp.itemComponent->ItemToSell()) != 0) {
		GameItem* gi = thisComp.itemComponent->GetItem(itemToSell);
		int value = gi->GetValue();
		int sold = MarketAI::Transact(gi,
			market->GetItemComponent(),	// buyer
			thisComp.itemComponent,		// seller
			0,							// no sales tax when selling to the market.
			true);

		if (sold)
			boughtStuff = true;
		else
			break;
	}
	if (boughtStuff) return;

	// Upgrades! Don't set ranged, shield, etc. because we don't want a critical followed 
	// by a non-critical. Also, personality probably should factor in to purchasing decisions.
	for( int i=0; i<3; ++i ) {
		const GameItem* purchase = 0;
		if ( i == 0 && ranged ) purchase = marketAI.HasRanged( thisComp.item->wallet.gold, ranged->GetValue() * 2 );
		if ( i == 1 && shield ) purchase = marketAI.HasShield( thisComp.item->wallet.gold, shield->GetValue() * 2 );
		if ( i == 2 && melee )  purchase = marketAI.HasMelee(  thisComp.item->wallet.gold, melee->GetValue()  * 2 );
		if ( purchase ) {
			
			MarketAI::Transact(	purchase,
								thisComp.itemComponent,
								market->GetItemComponent(),
								salesTax,
								true);
		}
	}
}


bool TaskList::UseFactory( const ComponentSet& thisComp, Chit* factory, int tech )
{
	// Guarentee we can build something:
	if ( thisComp.item->wallet.crystal[0] == 0 ) {
		return false;
	}
	if ( !thisComp.itemComponent->CanAddToInventory() ) {
		return false;
	}

	IRangedWeaponItem* ranged = thisComp.itemComponent->GetRangedWeapon(0);
	IMeleeWeaponItem* melee = thisComp.itemComponent->GetMeleeWeapon();
	IShield* shield = thisComp.itemComponent->GetShield();

	int itemType = 0;
	int subType = 0;
	int parts = 1;		// always have body.
	Random& random = thisComp.chit->random;

	int partsArr[]   = { WeaponGen::GUN_CELL, WeaponGen::GUN_DRIVER, WeaponGen::GUN_SCOPE };
	int effectsArr[] = { GameItem::EFFECT_FIRE, GameItem::EFFECT_SHOCK };	// don't allow explosive to be manufactured, yet: , GameItem::EFFECT_EXPLOSIVE

	// Get the inital to "everything"
	int effects = 0;
	for (int i = 0; i < GL_C_ARRAY_SIZE(effectsArr); ++i) {
		effects |= effectsArr[i];
	}

	if ( !ranged )		itemType = ForgeScript::GUN;
	else if ( !shield ) itemType = ForgeScript::SHIELD;
	else if ( !melee )	itemType = ForgeScript::SHIELD;
	else {
		itemType = random.Rand( ForgeScript::NUM_ITEM_TYPES );
	}

	if ( itemType == ForgeScript::GUN ) {
		if ( tech == 0 )
			subType = random.Rand( ForgeScript::NUM_TECH0_GUNS );
		else
			subType = random.Rand( ForgeScript::NUM_GUN_TYPES );
		parts = WeaponGen::PART_MASK;
	}
	else if ( itemType == ForgeScript::RING ) {
		if ( (tech < 2) && random.Bit() ) {
			parts = WeaponGen::RING_GUARD | WeaponGen::RING_TRIAD | WeaponGen::RING_BLADE;
		}
		else {
			// Don't use the blade at higher tech levels.
			parts = WeaponGen::RING_GUARD | WeaponGen::RING_TRIAD;
		}
	}

	random.ShuffleArray(partsArr, GL_C_ARRAY_SIZE(partsArr));
	random.ShuffleArray(effectsArr, GL_C_ARRAY_SIZE(effectsArr));

	int seed = thisComp.chit->ID() ^ thisComp.item->Traits().Experience();
	int level = thisComp.item->Traits().Level();
	ForgeScript forge( seed, level, tech );
	GameItem* item = new GameItem();

	int cp = 0;
	int ce = 0;
	Wallet wallet;

	// Given we have a crystal, we should always be able to build something.
	// Remove sub-parts and effects until we succeed. As the forge 
	// changes, this is no longer a hard rule.
	int maxIt = 10;
	while( maxIt ) {
		wallet.EmptyWallet();
		int techRequired = 0;
		forge.Build( itemType, subType, parts, effects, item, &wallet, &techRequired, true );

		if ( wallet <= thisComp.item->wallet && techRequired <= tech ) {
			break;
		}

		bool didSomething = false;
		if (wallet > thisComp.item->wallet && ce < GL_C_ARRAY_SIZE(effectsArr)) {
			effects &= ~(effectsArr[ce++]);
			didSomething = true;
		}
		if (techRequired > tech && cp < GL_C_ARRAY_SIZE(partsArr)) {
			parts &= ~(partsArr[cp++]);
			didSomething = true;
		}

		// The equations between cost are more complex; the
		// model above doesn't capture all the combos any more.
		// Make sure something changes every loop.
		if (!didSomething) {
			if (ce < GL_C_ARRAY_SIZE(effectsArr)) {
				effects &= ~(effectsArr[ce++]);
			}
			if (cp < GL_C_ARRAY_SIZE(partsArr)) {
				parts &= ~(partsArr[cp++]);
			}
		}
		--maxIt;
	}
	GLASSERT(maxIt);	// should have seen success...
	if (!maxIt) return false;

	Transfer(&item->wallet, &thisComp.item->wallet, wallet);
	thisComp.itemComponent->AddToInventory( item );
	thisComp.itemComponent->AddCraftXP( wallet.NumCrystals() );
	thisComp.item->historyDB.Increment( "Crafted" );

	Vector2I sector = thisComp.spatial->GetPosition2DI();
	GLOUTPUT(( "'%s' forged the item '%s' at sector=%x,%x\n",
		thisComp.item->BestName(),
		item->BestName(),
		sector.x, sector.y ));

	NewsHistory* history = thisComp.chit->GetChitBag()->GetNewsHistory();
	NewsEvent news( NewsEvent::FORGED, thisComp.spatial->GetPosition2D(), item, thisComp.chit ); 
	history->Add( news );
	item->GetItem()->keyValues.Set( "destroyMsg", NewsEvent::UN_FORGED );

	return true;
}
