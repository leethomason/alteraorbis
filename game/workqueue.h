#ifndef WORKQUEUE_ALTERA_INCLUDED
#define WORKQUEUE_ALTERA_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../gamui/gamui.h"

class WorldMap;
class XStream;

class WorkQueue
{
public:
	WorkQueue( WorldMap* );
	~WorkQueue();

	enum {
		CLEAR,
		ICE
	};

	void Serialize( XStream* xs );
	void Add( int action, grinliz::Vector2I& pos );

private:
	struct QueueItem {
		void Serialize( XStream* xs );
		int action;
		grinliz::Vector2I pos;
	};

	WorldMap*						worldMap;
	grinliz::CDynArray< QueueItem > queue;
	grinliz::CDynArray< gamui::Image* > images;
};

#endif // WORKQUEUE_ALTERA_INCLUDED
