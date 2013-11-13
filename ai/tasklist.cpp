#include "tasklist.h"

#include "../game/lumosmath.h"
#include "../game/pathmovecomponent.h"
#include "../game/workqueue.h"
#include "../game/aicomponent.h"
#include "../game/worldmap.h"
#include "../game/worldgrid.h"
#include "../game/lumoschitbag.h"
#include "../game/lumosgame.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

#include "../script/corescript.h"
#include "../script/buildscript.h"

using namespace grinliz;
using namespace ai;

Vector2I TaskList::Pos2I() const
{
	Vector2I v = { 0, 0 };
	if ( !taskList.Empty() ) {
		v = taskList[0].pos2i;
	}
	return v;
}


void TaskList::Push( const Task& task )
{
	taskList.Push( task );
}


void TaskList::DoStanding( int time )
{
	if ( Standing() ) {
		taskList[0].timer -= time;
		if ( taskList[0].timer <= 0 ) {
			taskList.Remove(0);
		}
	}
}


void TaskList::DoTasks( Chit* chit, WorkQueue* workQueue, U32 delta, U32 since )
{
	if ( taskList.Empty() ) return;
	PathMoveComponent* pmc		= GET_SUB_COMPONENT( chit, MoveComponent, PathMoveComponent );
	SpatialComponent* spatial	= chit->GetSpatialComponent();
	AIComponent* ai				= chit->GetAIComponent();
	LumosChitBag* chitBag		= chit->GetLumosChitBag();
	ItemComponent* itemComp		= chit->GetItemComponent();

	if ( !pmc || !spatial || !ai || !itemComp ) {
		taskList.Clear();
		return;
	}

	Task* task        = &taskList[0];
	Vector2I pos2i    = spatial->GetPosition2DI();
	Vector2F taskPos2 = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
	Vector3F taskPos3 = { taskPos2.x, 0, taskPos2.y };

	// If this is a task associated with a work item, make
	// sure that work item still exists.
	if ( task->taskID ) {
		const WorkQueue::QueueItem* queueItem = 0;
		if ( workQueue ) {
			queueItem = workQueue->GetJobByTaskID( task->taskID );
		}
		if ( !queueItem ) {
			// This task it attached to a work item that expired.
			taskList.Clear();
			return;
		}
	}

	switch ( task->action ) {
	case Task::TASK_MOVE:
		if ( pmc->Stopped() ) {
			if ( pos2i == task->pos2i ) {
				// arrived!
				taskList.Remove(0);
			}
			else {
				Vector2F dest = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
				ai->Move( dest, false );
			}
		}
		break;

	case Task::TASK_STAND:
		if ( pmc->Stopped() ) {
			task->timer -= (int)since;
			if ( task->timer <= 0 ) {
				taskList.Remove(0);
			}
			if ( taskList.Size() >= 2 ) {
				int action = taskList[1].action;
				if ( action == Task::TASK_BUILD || action == Task::TASK_REMOVE ) {
					int debug=1;
				}
			}
		}
		break;

	case Task::TASK_REMOVE:
		{
			//GLASSERT( workQueue && queueItem );
			//if ( workQueue->TaskCanComplete( *queueItem )) {
			//worldMap, chitBag, item.pos, item.action != CLEAR, CalcTaskSize( item )
			if ( WorkQueue::TaskCanComplete( worldMap, chitBag, task->pos2i, false, WorkQueue::CalcTaskSize( task->structure ))) {
				const WorldGrid& wg = worldMap->GetWorldGrid( task->pos2i.x, task->pos2i.y );
				if ( wg.RockHeight() ) {
					DamageDesc dd( 10000, 0 );	// FIXME need constant
					Vector3I voxel = { task->pos2i.x, 0, task->pos2i.y };
					worldMap->VoxelHit( voxel, dd );
				}
				else {
					Chit* found = chitBag->QueryRemovable( task->pos2i );
					if ( found ) {
						DamageDesc dd( 10000, 0 );
						ChitDamageInfo info( dd );
						info.originID = 0;
						info.awardXP  = false;
						info.isMelee  = true;
						info.isExplosion = false;
						info.originOfImpact = spatial->GetPosition();
						found->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &info ), 0 );
					}
				}
				if ( wg.Pave() ) {
					worldMap->SetPave( task->pos2i.x, task->pos2i.y, 0 );
				}
				taskList.Remove(0);
			}
			else {
				taskList.Clear();
			}
		}
		break;

	case Task::TASK_BUILD:
		{
			// IsPassable (isLand, noRocks)
			// No plants
			// No buildings
			BuildScript buildScript;
			const BuildData& buildData	= buildScript.GetDataFromStructure( task->structure );
			int cost					= buildData.cost;
			bool canAfford				= itemComp->GetItem(0)->wallet.gold >= cost;

			if (    canAfford 
				 && WorkQueue::TaskCanComplete( worldMap, chitBag, task->pos2i, true, WorkQueue::CalcTaskSize( task->structure ))) 
			{
				if ( task->structure == IStringConst::ice ) {
					worldMap->SetRock( task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE );
				}
				else if ( task->structure == IStringConst::pave ) {
					worldMap->SetPave( task->pos2i.x, task->pos2i.y, chit->PrimaryTeam()%3+1 );
				}
				else {
					// Move the build cost to the building. The gold is held there until the
					// building is destroyed
					itemComp->GetItem(0)->wallet.gold -= cost;
					Chit* building = chitBag->NewBuilding( task->pos2i, task->structure.c_str(), chit->PrimaryTeam() );
					building->GetItem()->wallet.AddGold( cost );
				}
				taskList.Remove(0);
			}
			else {
				// Plan has gone bust:
				taskList.Clear();
			}
		}
		break;

	case Task::TASK_PICKUP:
		{
			int chitID = task->data;
			Chit* itemChit = chitBag->GetChit( chitID );
			if (    itemChit 
				 && itemChit->GetSpatialComponent()
				 && itemChit->GetSpatialComponent()->GetPosition2DI() == spatial->GetPosition2DI()
				 && itemChit->GetItemComponent()->NumItems() == 1 )	// doesn't have sub-items / intrinsic
			{
				if ( itemComp->CanAddToInventory() ) {
					ItemComponent* ic = itemChit->GetItemComponent();
					itemChit->Remove( ic );
					chit->GetItemComponent()->AddToInventory( ic );
					chitBag->DeleteChit( itemChit );
				}
			}
			taskList.Remove(0);
		}
		break;

	case Task::TASK_USE_BUILDING:
		{
			Chit* building	= chitBag->QueryPorch( pos2i );
			Vector2I sector	= ToSector( pos2i );
			CoreScript* cs	= chitBag->GetCore( sector );

			if ( building ) {
				IString buildingName = building->GetItem()->name;

				if ( chit->PlayerControlled() ) {
					// Not sure how this happened. But do nothing.
				}
				// Workers:
				else if ( chit->GetItem()->flags & GameItem::AI_DOES_WORK ) {
					if ( buildingName == IStringConst::vault ) {
						GameItem* vaultItem = building->GetItem();
						GLASSERT( vaultItem );
						Wallet w = vaultItem->wallet.EmptyWallet();
						vaultItem->wallet.Add( w );

						// Put everything in the vault.
						ItemComponent* vaultIC = building->GetItemComponent();
						for( int i=1; i<itemComp->NumItems(); ++i ) {
							if ( vaultIC->CanAddToInventory() ) {
								GameItem* item = itemComp->RemoveFromInventory(i);
								if ( item ) {
									vaultIC->AddToInventory( item );
								}
							}
						}

						// Move gold & crystal to the owner?
						Chit* controller = cs->GetAttached( 0 );
						if ( controller && controller->GetItemComponent() ) {
							Wallet w = vaultItem->wallet.EmptyWallet();
							controller->GetItem()->wallet.Add( w );
						}
					}
				}
			}
			taskList.Remove(0);
		}
		break;

	default:
		GLASSERT( 0 );
		break;

	}
}
