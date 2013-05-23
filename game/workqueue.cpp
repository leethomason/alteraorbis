#include "workqueue.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace gamui;

WorkQueue::WorkQueue( WorldMap* wm ) : worldMap( wm )
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
	RenderAtom atom = LumosGame::CalcIconAtom( 10+item.action, true );	// fixme. switch to new icon texture
	Image* image = new Image( &worldMap->overlay1, atom, true );
	images.Push( image );
	image->SetSize( 1, 1 );
	image->SetPos( (float)item.pos.x, (float)item.pos.y );

}


void WorkQueue::Add( int action, grinliz::Vector2I& pos )
{
	QueueItem item = { action, pos };
	queue.Push( item );
	InitImage( item );
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



