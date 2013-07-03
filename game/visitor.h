#ifndef LUMOS_VISITOR_INCLUDED
#define LUMOS_VISITOR_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "sectorport.h"

class XStream;
class WorldMap;

// Each visitor has their own personality and memory, seperate
// from the chit AI.
struct VisitorData
{
	VisitorData() : id( 0 ) {}
	void Serialize( XStream* xs );

	void Connect() {
		sectorVisited.Clear();
		kioskTime = 0;
	}

	enum {	NUM_VISITS = 4, 			// how many domains would like to be visited before disconnect
			KIOSK_TIME = 5000
	};
	int id;								// chit id, and whether in-world or not.
	U32 kioskTime;						// time spent standing at current kiosk
	grinliz::CArray< grinliz::Vector2I, NUM_VISITS > sectorVisited;	// which sectors we have visited a kiosk at. 
};


class Visitors
{
public:
	Visitors();
	~Visitors();

	void Serialize( XStream* xs );

	enum { NUM_VISITORS = 200 };
	VisitorData visitorData[NUM_VISITORS];

	static VisitorData* Get( int index );
	// only returns existing; doesn't create.
	static Visitors* Instance()	{ return instance; }

	SectorPort ChooseDestination( int visitorIndex, WorldMap* map );

private:
	static Visitors* instance;
};

#endif // LUMOS_VISITOR_INCLUDED
