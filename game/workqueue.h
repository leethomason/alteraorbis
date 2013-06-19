#ifndef WORKQUEUE_ALTERA_INCLUDED
#define WORKQUEUE_ALTERA_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../gamui/gamui.h"

class WorldMap;
class XStream;
class LumosChitBag;
class Model;
class Engine;

class WorkQueue
{
public:
	WorkQueue( WorldMap*, LumosChitBag*, Engine* );
	~WorkQueue();

	enum {
		NO_ACTION,
		CLEAR_GRID,
		BUILD_ICE,
		NUM_ACTIONS
	};

	struct QueueItem {
		QueueItem() : action(NO_ACTION), building(0), assigned(0) { pos.Zero(); }
		QueueItem( int p_action, const grinliz::Vector2I& p_pos ) : action(p_action), pos(p_pos), assigned(0) {}

		void Serialize( XStream* xs );

		int					action;		// CLEAR_GRID, etc.
		int					building;	// what to construct
		grinliz::Vector2I	pos;
		int					assigned;	// id of worker assigned this task.			
	};

	void Serialize( XStream* xs );
	void Add( int action, const grinliz::Vector2I& pos );		// add an action to do
	
	const QueueItem*	Find( const grinliz::Vector2I& chitPos );	// find something to do. don't hold pointer!
	void				Assign( int id, const QueueItem* item );	// associate this chit with a job.
	const QueueItem*	GetJob( int chitID );						// get the current job, don't hold pointer!
	void				ReleaseJob( int chitID );

	void DoTick();	// mostly looks for work being complete.
	const grinliz::CDynArray< WorkQueue::QueueItem >& Queue() const { return queue; };	

private:

	void AddImage( const QueueItem& item );
	void RemoveImage( const QueueItem& item );

	Engine*			engine;
	WorldMap*		worldMap;
	LumosChitBag*	chitBag;
	grinliz::CDynArray< QueueItem >		queue;	// work to do
	grinliz::CDynArray< gamui::Image* > images;	// images to tag the work
	grinliz::CDynArray< Model* >		models;	// models to tag the work
};

#endif // WORKQUEUE_ALTERA_INCLUDED
