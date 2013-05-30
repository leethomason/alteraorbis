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
		CLEAR_GRID,
		BUILD_ICE
	};

	void Serialize( XStream* xs );
	void Add( int action, grinliz::Vector2I& pos );
	void DoTick();	// mostly looks for work being complete.

	struct QueueItem {
		void Serialize( XStream* xs );
		int action;
		grinliz::Vector2I pos;
	};
	const grinliz::CDynArray< WorkQueue::QueueItem >& Queue() const { return queue; };	

private:

	void InitImage( const QueueItem& item );

	WorldMap*		worldMap;
	LumosChitBag*	chitBag;
	grinliz::CDynArray< QueueItem > queue;
	grinliz::CDynArray< gamui::Image* > images;
};

#endif // WORKQUEUE_ALTERA_INCLUDED
