#ifndef ALTERA_NEWS_INCLUDED
#define ALTERA_NEWS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "gamelimits.h"
#include "lumosmath.h"

class XStream;
class WorldMap;
class ChitBag;

// Lots of these - needs to be small. These are kept for the entire
// record of the game. 
class NewsEvent {
	friend class NewsHistory;
	
public:
	NewsEvent() { Clear(); }
	NewsEvent( U32 what, const grinliz::Vector2F& pos, int chitID=0, int altChitID=0 ) {
		Clear();
		this->what = what;
		this->pos = pos;
		this->chitID = chitID;
		this->altChitID = altChitID;
	}

	void Serialize( XStream* xs );

	enum {
		DENIZEN_CREATED = 1,
		DENIZEN_KILLED,
		GREATER_MOB_CREATED,
		GREATER_MOB_KILLED,

		SECTOR_HERD,		

		VOLCANO,
		POOL,
		WATERFALL,

		NUM_WHAT
	};

	grinliz::IString GetWhat() const;
	void Console( grinliz::GLString* str, ChitBag* chitBag ) const;
	grinliz::Vector2I Sector() const { return ToSector( ToWorld2I( pos )); }

	int					what;	
	grinliz::Vector2F	pos;			// where it happened
	int					chitID;			// whom the news in about (subject)
	int					altChitID;		// secondary player
	U32					date;			// when it happened, in msec

	void Clear() {
		what = 0;
		pos.Zero();
		chitID = 0;
		altChitID = 0;
		date = 0;
	}
};


// "scoped singleton": created and destroyed by a main game/scene scope, 
// but there can only be one active. (Could implement a stack, in the future.)
// Created and destroyed by the Simulation
class NewsHistory
{
public:
	NewsHistory();
	~NewsHistory();

	static NewsHistory* Instance() { return instance; }
	void Add( const NewsEvent& event );

	// First thing to tick! Updates the main clock.
	void DoTick( U32 delta );
	void Serialize( XStream* xs );

	// Time
	int    AgeI() const { return date / AGE_IN_MSEC; }
	double AgeD() const { return double(date) / double(AGE_IN_MSEC); }
	float  AgeF() const { return float(AgeD()); }

	int NumNews() const { return events.Size(); }
	const NewsEvent& News( int i ) { GLASSERT( i >= 0 && i < events.Size() ); return events[i]; }
	const NewsEvent* NewsPtr() { return events.Mem(); }

private:

	static NewsHistory* instance;

	U32 date;
	grinliz::CDynArray< NewsEvent > events;
};


#endif // ALTERA_NEWS_INCLUDED
