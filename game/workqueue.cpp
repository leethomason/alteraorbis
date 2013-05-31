#include "workqueue.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "lumoschitbag.h"
#include "gameitem.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace gamui;

static const float NOTIFICATION_RAD = 5.0f;

WorkQueue::WorkQueue( WorldMap* wm, LumosChitBag* lcb ) : worldMap( wm ), chitBag( lcb )
{
}


WorkQueue::~WorkQueue()
{
	while( !images.Empty() ) {
		Image* image = images.Pop();
		delete image;
	}
}


void WorkQueue::AddImage( const QueueItem& item )
{
	RenderAtom atom = LumosGame::CalcIconAtom( 11+item.action, true );	// fixme. switch to new icon texture
	Image* image = new Image( &worldMap->overlay1, atom, true );
	images.Push( image );
	image->SetSize( 1, 1 );
	image->SetPos( (float)item.pos.x, (float)item.pos.y );
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
	GLASSERT( 0 );
}


void WorkQueue::Add( int action, grinliz::Vector2I& pos2i )
{
	QueueItem item = { action, pos2i };
	queue.Push( item );
	AddImage( item );

	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	// Notify near.
	CChitArray array;
	chitBag->QuerySpatialHash( &array, pos2, NOTIFICATION_RAD, 0, LumosChitBag::WorkerFilter );
	for( int i=0; i<array.Size(); ++i ) {
		ChitMsg msg( ChitMsg::WORKQUEUE_UPDATE );
		array[i]->SendMessage( msg );
	}
}


void WorkQueue::DoTick()
{
	for( int i=0; i<queue.Size(); ++i ) {
		switch ( queue[i].action )
		{
		case CLEAR_GRID:
			{
				const WorldGrid& wg = worldMap->GetWorldGrid( queue[i].pos.x, queue[i].pos.y );
				if ( wg.RockHeight() == 0 ) {
					RemoveImage( queue[i] );
					queue.Remove( i );
					--i;
				}
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
	XARC_SER( xs, pos );
	XarcClose( xs );
}


void WorkQueue::Serialize( XStream* xs ) 
{
	XarcOpen( xs, "WorkQueue" );
	XARC_SER_CARRAY( xs, queue );

	if ( xs->Loading() ) {
		for( int i=0; i<queue.Size(); ++i ) {
			AddImage( queue[i] );
		}
	}
	XarcClose( xs );
}



