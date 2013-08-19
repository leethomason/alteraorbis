#include "workqueue.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "lumoschitbag.h"
#include "gameitem.h"

#include "../engine/engine.h"

#include "../xegame/istringconst.h"

#include "../xarchive/glstreamer.h"

#include "../script/itemscript.h"

using namespace grinliz;
using namespace gamui;

static const float NOTIFICATION_RAD = 5.0f;

WorkQueue::WorkQueue( WorldMap* wm, LumosChitBag* lcb, Engine* e ) : worldMap( wm ), chitBag( lcb ), engine( e )
{
	sector.Zero();
	idPool = 0;
}


WorkQueue::~WorkQueue()
{
	for( int i=0; i<queue.Size(); ++i ) {
		Model* m = queue[i].model;
		if ( m ) {
			engine->FreeModel( m );
		}
	}
}


int WorkQueue::CalcTaskSize( const QueueItem& item )
{
	int size = 1;
	if ( !item.structure.empty() ) {
		const GameItem& gameItem = ItemDefDB::Instance()->Get( item.structure.c_str() );
		gameItem.GetValue( "size", &size );
	}
	return size;
}


void WorkQueue::AddImage( QueueItem* item )
{
	const char* name = 0;
	Vector3F pos = { 0, 0, 0 };
	int size = CalcTaskSize( *item );

	if ( item->action == CLEAR ) {
		if ( item->structure.empty() ) {
			// Clearing ice or plant
			const WorldGrid& wg = worldMap->GetWorldGrid( item->pos.x, item->pos.y );
			switch ( wg.Height() ) {
			case 1:	name = "clearMarker1";	break;
			case 2: name = "clearMarker2";	break;
			case 3:	name = "clearMarker3";	break;
			}
			pos.Set( (float)item->pos.x + 0.5f, 0, (float)item->pos.y+0.5f );
		}
		else {
			if ( size == 1 ) {
				name = "clearMarker1";
				pos.Set( (float)item->pos.x+0.5f, 0, (float)item->pos.y+0.5f );
			}
			else {
				name = "clearMarker2";
				pos.Set( (float)item->pos.x+1.f, 0, (float)item->pos.y+1.f );
			}
		}
	}
	else {
		if ( size == 1 ) {
			name = "buildMarker1";
			pos.Set( (float)item->pos.x + 0.5f, 0, (float)item->pos.y+0.5f );
		}
		else {
			name = "buildMarker2";
			pos.Set( (float)item->pos.x + 1.f, 0, (float)item->pos.y+1.f );
		}
	}
	GLASSERT( name );
	if ( name ) {
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( name );
		GLASSERT( res );
		Model* m = engine->AllocModel( res );
		GLASSERT( m );
		m->SetFlag( MODEL_CLICK_THROUGH );

		m->SetPos( pos );
		GLASSERT( item->model == 0 );
		item->model = m;
	}
	GLASSERT( item->model );
}


void WorkQueue::RemoveImage( QueueItem* item )
{
	if ( item->model ) {
		engine->FreeModel( item->model );
		item->model = 0;
		return;
	}
	GLASSERT( 0 );
}


void WorkQueue::Remove( const grinliz::Vector2I& pos )
{
	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].pos == pos ) {
			RemoveItem( i );
		}
	}
}



void WorkQueue::Add( int action, const grinliz::Vector2I& pos2i, IString structure )
{
	GLASSERT( action >= CLEAR && action < NUM_ACTIONS );

	if ( pos2i.x / SECTOR_SIZE == sector.x && pos2i.y / SECTOR_SIZE == sector.y ) {
		// okay!
	}
	else {
		// wrong sector.
		return;
	}

	// Clear out existing.
	Remove( pos2i );

	QueueItem item;
	item.action = action;
	item.pos = pos2i;
	item.structure = structure;
	item.taskID = ++idPool;

	AddImage( &item );
	queue.Push( item );

	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	// Notify near.
	CChitArray array;
	ItemNameFilter workerFilter( IStringConst::kworker );
	chitBag->QuerySpatialHash( &array, pos2, NOTIFICATION_RAD, 0, &workerFilter );
	for( int i=0; i<array.Size(); ++i ) {
		ChitMsg msg( ChitMsg::WORKQUEUE_UPDATE );
		array[i]->SendMessage( msg );
	}
}


void WorkQueue::Assign( int id, const WorkQueue::QueueItem* item )
{
	int index = item - queue.Mem();
	GLASSERT( index >= 0 && index < queue.Size() );
	GLASSERT( queue[index].assigned == 0 );
	queue[index ].assigned = id;
}


void WorkQueue::ReleaseJob( int chitID )
{
	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].assigned == chitID ) {
			queue[i].assigned = 0;
		}
	}
}


void WorkQueue::ClearJobs()
{
	while( queue.Size() ) {
		RemoveItem( queue.Size()-1 );
	}
}


const WorkQueue::QueueItem* WorkQueue::GetJobByTaskID( int taskID )
{
	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].taskID == taskID ) {
			GLASSERT( queue[i].model );
			return &queue[i];
		}
	}
	return 0;
}


const WorkQueue::QueueItem* WorkQueue::GetJob( int id )
{
	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].assigned == id ) {
			GLASSERT( queue[i].model );
			return &queue[i];
		}
	}
	return 0;
}


const WorkQueue::QueueItem* WorkQueue::Find( const grinliz::Vector2I& chitPos )
{
	int best=-1;
	float bestCost = FLT_MAX;
	const Vector2F start = { (float)chitPos.x+0.5f, (float)chitPos.y+0.5f };

	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].assigned == 0 ) {

			float cost = 0;
			Vector2F end = { (float)queue[i].pos.x+0.5f, (float)queue[i].pos.y+0.5f };

			if ( queue[i].action == CLEAR ) {
				Vector2F bestEnd = { 0, 0 };

				if ( worldMap->CalcPathBeside( start, end, &bestEnd, &cost )) {
					if ( cost < bestCost ) {
						bestCost = cost;
						best = i;
					}
				}
			}
			else {
				if ( worldMap->CalcPath( start, end, 0, 0, 0, &cost )) {
					if ( cost < bestCost ) {
						bestCost = cost;
						best = i;
					}
				}
			}
		}
	}
	if ( best >= 0 ) {
		return &queue[best];
	}
	return 0;
}


void WorkQueue::RemoveItem( int index )
{
	GLASSERT( queue[index].model );
	RemoveImage( &queue[index] );
	queue.Remove( index );
}


bool WorkQueue::TaskCanComplete( const QueueItem& item )
{
	Vector2I pos2i = { item.pos.x, item.pos.y };
	Vector2F pos2  = { (float)pos2i.x + 0.5f, (float)pos2i.y+0.5f };
	const WorldGrid& wg = worldMap->GetWorldGrid( pos2i.x, pos2i.x );
	RemovableFilter removableFilter;
	int size = CalcTaskSize( item );

	int passable = 0;
	int removable = 0;
	for( int y=pos2i.y; y<pos2i.y+size; ++y ) {
		for( int x=pos2i.x; x<pos2i.x+size; ++x ) {
			if ( worldMap->IsPassable( x, y )) {
				++passable;
			}
			Vector2I v = { x, y };
			if ( chitBag->QueryRemovable( v )) {
				++removable;
			}
		}
	}

	switch ( item.action )
	{
	case CLEAR:
		if ( passable == size*size && removable == 0 ) {
			// nothing to clear.
			return false;
		}
		break;

	case BUILD:
		if ( passable < size*size || removable ) {
			// stuff in the way
			return false;
		}
		break;

	default:
		GLASSERT( 0 );
		break;
	}
	return true;
}

void WorkQueue::DoTick()
{
	for( int i=0; i<queue.Size(); ++i ) {

		if ( queue[i].assigned ) {
			if ( !chitBag->GetChit( queue[i].assigned )) {
				queue[i].assigned = 0;
			}
		}

		if ( !TaskCanComplete( queue[i] )) {
			RemoveItem( i );
			--i;
		}
	}
}


void WorkQueue::QueueItem::Serialize( XStream* xs )
{
	XarcOpen( xs, "QueueItem" );
	XARC_SER( xs, action );
	XARC_SER( xs, structure );
	XARC_SER( xs, pos );
	XARC_SER( xs, assigned );
	XARC_SER( xs, taskID );
	XarcClose( xs );
}


void WorkQueue::Serialize( XStream* xs ) 
{
	XarcOpen( xs, "WorkQueue" );
	XARC_SER( xs, sector );
	XARC_SER_CARRAY( xs, queue );

	if ( xs->Loading() ) {
		for( int i=0; i<queue.Size(); ++i ) {
			AddImage( &queue[i] );
			idPool = Max( idPool, queue[i].taskID+1 );
		}
	}
	XarcClose( xs );
}



