#ifndef WORKQUEUE_ALTERA_INCLUDED
#define WORKQUEUE_ALTERA_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../gamui/gamui.h"

class WorldMap;
class XStream;
class LumosChitBag;

class WorkQueue
{
public:
	WorkQueue( WorldMap*, LumosChitBag* );
	~WorkQueue();

	enum {
		NO_ACTION,
		CLEAR_GRID,
		BUILD_ICE,
		NUM_ACTIONS
	};

	struct QueueItem {
		QueueItem() : action(NO_ACTION), assigned(0) { pos.Zero(); }
		QueueItem( int p_action, const grinliz::Vector2I& p_pos ) : action(p_action), pos(p_pos), assigned(0) {}

		void Serialize( XStream* xs );

		int					action;		// CLEAR_GRID, etc.
		grinliz::Vector2I	pos;
		int					assigned;	// id of worker assigned this task.			
	};

	void Serialize( XStream* xs );
	void Add( int action, const grinliz::Vector2I& pos );		// add an action to do
	const QueueItem* Find( const grinliz::Vector2I& chitPos );	// find something to do. don't hold pointer!
	void Assign( int id, const QueueItem* item );
	const QueueItem* GetJob( int id );							// don't hold pointer!

	void DoTick();	// mostly looks for work being complete.
	const grinliz::CDynArray< WorkQueue::QueueItem >& Queue() const { return queue; };	

private:

	void AddImage( const QueueItem& item );
	void RemoveImage( const QueueItem& item );

	WorldMap*		worldMap;
	LumosChitBag*	chitBag;
	grinliz::CDynArray< QueueItem > queue;
	grinliz::CDynArray< gamui::Image* > images;
};

#endif // WORKQUEUE_ALTERA_INCLUDED
