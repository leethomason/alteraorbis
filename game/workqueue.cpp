#include "workqueue.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "lumoschitbag.h"
#include "gameitem.h"

#include "../engine/engine.h"

#include "../xegame/istringconst.h"

#include "../xarchive/glstreamer.h"

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
	while( !images.Empty() ) {
		Image* image = images.Pop();
		delete image;
	}
	while ( !models.Empty() ) {
		Model* m = models.Pop();
		engine->FreeModel( m );
	}
}


void WorkQueue::AddImage( const QueueItem& item )
{
	if ( item.action == CLEAR ) {
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "unitPlateCentered" );
		GLASSERT( res );
		Model* m = engine->AllocModel( res );
		m->SetFlag( MODEL_CLICK_THROUGH );
		GLASSERT( m );
		const WorldGrid& wg = worldMap->GetWorldGrid( item.pos.x, item.pos.y );
		int h = wg.Height();
		m->SetPos( (float)item.pos.x + 0.5f, (float)h + 0.01f, (float)item.pos.y+0.5f );
		models.Push( m );
	}
	else {
		RenderAtom atom = LumosGame::CalcIconAtom( 10+item.action, true );	// fixme. switch to new icon texture
		Image* image = new Image( &worldMap->overlay1, atom, true );
		images.Push( image );
		image->SetSize( 1, 1 );
		image->SetPos( (float)item.pos.x, (float)item.pos.y );
	}

}


void WorkQueue::RemoveImage( const QueueItem& item )
{
	for( int i=0; i<images.Size(); ++i ) {
		Vector2I v = { (int)images[i]->X(), (int)images[i]->Y() };
		if ( v == item.pos ) {
			delete images[i];
			images.Remove(i);
			return;
		}
	}
	for( int i=0; i<models.Size(); ++i ) {
		Vector2I v = { (int)models[i]->Pos().x, (int)models[i]->Pos().z };
		if ( v == item.pos ) {
			engine->FreeModel( models[i] );
			models.Remove(i);
			return;
		}
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

	QueueItem item( action, pos2i, structure, ++idPool );
	queue.Push( item );
	AddImage( item );

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
			return &queue[i];
		}
	}
	return 0;
}


const WorkQueue::QueueItem* WorkQueue::GetJob( int id )
{
	for( int i=0; i<queue.Size(); ++i ) {
		if ( queue[i].assigned == id ) {
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
	RemoveImage( queue[index] );
	queue.Remove( index );
}


void WorkQueue::DoTick()
{
	for( int i=0; i<queue.Size(); ++i ) {

		if ( queue[i].assigned ) {
			if ( !chitBag->GetChit( queue[i].assigned )) {
				queue[i].assigned = 0;
			}
		}

		Vector2I pos2i = { queue[i].pos.x, queue[i].pos.y };
		Vector2F pos2  = { (float)pos2i.x + 0.5f, (float)pos2i.y+0.5f };
		const WorldGrid& wg = worldMap->GetWorldGrid( pos2i.x, pos2i.x );
		RemovableFilter removableFilter;

		switch ( queue[i].action )
		{
		case CLEAR:
			if ( worldMap->IsPassable( pos2i.x, pos2i.y ) && !chitBag->QueryRemovable( pos2i )) {
				RemoveItem( i );
				--i;
			}
			break;

		case BUILD:
			if ( !worldMap->IsPassable( pos2i.x, pos2i.y ) || chitBag->QueryBuilding( pos2i )) {
				RemoveItem( i );
				--i;
			}
			break;

		default:
			GLASSERT( 0 );
			break;
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
			AddImage( queue[i] );
			idPool = Max( idPool, queue[i].taskID+1 );
		}
	}
	XarcClose( xs );
}



