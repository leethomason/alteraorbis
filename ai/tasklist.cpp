#include "tasklist.h"
#include "../game/pathmovecomponent.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../game/workqueue.h"
#include "../game/aicomponent.h"
#include "../game/worldmap.h"
#include "../game/worldgrid.h"
#include "../game/lumoschitbag.h"
#include "../game/lumosgame.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../scenes/characterscene.h"

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


void TaskList::DoTasks( Chit* chit, WorkQueue* workQueue, WorldMap* map )
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
			// DoStand will decrement timer.
			if ( task->timer <= 0 ) {
				taskList.Remove(0);
			}
		}
		else {
			ai->Stand();
		}
		break;

	case Task::TASK_REMOVE:
		{
			//GLASSERT( workQueue && queueItem );
			//if ( workQueue->TaskCanComplete( *queueItem )) {
			//worldMap, chitBag, item.pos, item.action != CLEAR, CalcTaskSize( item )
			if ( WorkQueue::TaskCanComplete( map, chitBag, task->pos2i, false, WorkQueue::CalcTaskSize( task->structure ))) {
				const WorldGrid& wg = map->GetWorldGrid( task->pos2i.x, task->pos2i.y );
				if ( wg.RockHeight() ) {
					DamageDesc dd( 10000, 0 );	// FIXME need constant
					Vector3I voxel = { task->pos2i.x, 0, task->pos2i.y };
					map->VoxelHit( voxel, dd );
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
					map->SetPave( task->pos2i.x, task->pos2i.y, 0 );
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
			if ( WorkQueue::TaskCanComplete( map, chitBag, task->pos2i, false, WorkQueue::CalcTaskSize( task->structure ))) {
			//if ( workQueue->TaskCanComplete( *queueItem )) {
				if ( task->structure.empty() ) {
					map->SetRock( task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE );
				}
				else {
					if ( task->structure == "pave" ) {
						map->SetPave( task->pos2i.x, task->pos2i.y, chit->PrimaryTeam()%3+1 );
					}
					else {
						chitBag->NewBuilding( task->pos2i, task->structure.c_str(), chit->PrimaryTeam() );
					}
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
			Chit* building = chitBag->QueryPorch( pos2i );
			if ( building ) {
				if ( chit->PlayerControlled() ) {
					chitBag->PushScene( LumosGame::SCENE_CHARACTER, 
										new CharacterSceneData( itemComp, building->GetItemComponent() ));
				}
				// Workers:
				else if ( chit->GetItem()->flags & GameItem::AI_DOES_WORK ) {
					if ( building->GetItem()->name == IStringConst::vault ) {
						ItemComponent* vaultIC = building->GetItemComponent();
						GLASSERT( vaultIC );
						Wallet w = itemComp->EmptyWallet();
						vaultIC->AddGold( w );

						// Put everything in the vault.
						for( int i=1; i<itemComp->NumItems(); ++i ) {
							if ( vaultIC->CanAddToInventory() ) {
								GameItem* item = itemComp->RemoveFromInventory(i);
								if ( item ) {
									vaultIC->AddToInventory( item );
								}
							}
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
