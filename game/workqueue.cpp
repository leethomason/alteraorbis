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


void WorkQueue::InitImage( const QueueItem& item )
{
	RenderAtom atom = LumosGame::CalcIconAtom( 11+item.action, true );	// fixme. switch to new icon texture
	Image* image = new Image( &worldMap->overlay1, atom, true );
	images.Push( image );
	image->SetSize( 1, 1 );
	image->SetPos( (float)item.pos.x, (float)item.pos.y );

}


void WorkQueue::Add( int action, grinliz::Vector2I& pos2i )
{
	QueueItem item = { action, pos2i };
	queue.Push( item );
	InitImage( item );

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
			InitImage( queue[i] );
		}
	}
	XarcClose( xs );
}



