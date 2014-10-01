#ifndef ALTERA_NEWS_INCLUDED
#define ALTERA_NEWS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "gamelimits.h"
#include "lumosmath.h"
#include "../xegame/stackedsingleton.h"

class XStream;
class WorldMap;
class ChitBag;
class Chit;
class GameItem;

// Lots of these - needs to be small. These are kept for the entire
// record of the game. 
class NewsEvent {
	friend class NewsHistory;
	
public:
	NewsEvent() { Clear(); }
	NewsEvent( U32 what, const grinliz::Vector2F& pos, Chit* main=0, Chit* second=0 );
	NewsEvent( U32 what, const grinliz::Vector2F& pos, const GameItem* item, Chit* second=0 );

	void Serialize( XStream* xs );

public:
	// Be sure to update GetWhat() when this list chnages.
	enum {							//	FIRST		SECOND		CHIT
		NONE,
		DENIZEN_CREATED ,			//	created					created		
		DENIZEN_KILLED,				//  killed		killer		killed
		GREATER_MOB_CREATED,		//  created					created		
		GREATER_MOB_KILLED,			//  killed		killer		killed
		DOMAIN_CREATED,				//  domain					domain
		DOMAIN_DESTROYED,			//  domain		killer		domain	
		ROQUE_DENIZEN_JOINS_TEAM,	// denizen					denizen

		FORGED,						//	item		maker		maker		
		UN_FORGED,					//  item		killer	

		PURCHASED,					//	item		purchaser	0
		STARVATION,					//  victim					victim
		BLOOD_RAGE,					//  victim					victim
		VISION_QUEST,				//	victim					victim
		GREATER_SUMMON_TECH,		//  mob
		DOMAIN_CONQUER,				//	conquered	conquerer	conquered

		// Current events, but not logged:
		START_CURRENT,
		SECTOR_HERD,				//	mob
		RAMPAGE,					//	mob

		VOLCANO,
		POOL,
		WATERFALL,

		NUM_WHAT
	};

	bool				Origin() const { return    what == DENIZEN_CREATED
												|| what == GREATER_MOB_CREATED
												|| what == DOMAIN_CREATED
												|| what == FORGED; }
	grinliz::IString	GetWhat() const;
	void				Console( grinliz::GLString* str, ChitBag*, int shortNameForThisID ) const;

	int What() const { return what; }
	const grinliz::Vector2F& Pos() const { return pos; }
	grinliz::Vector2I	Sector() const { return ToSector( ToWorld2I( pos )); }

	int	FirstChitID() const { return chitID; }
	int FirstItemID() const { return itemID; }
	int SecondItemID() const { return secondItemID; }

	static grinliz::IString IDToName( int id, bool shortName );

private:
	int					what;	
	grinliz::Vector2F	pos;			// where it happened
	// Use Items instead of Chits so we can look up history.
	int					chitID;			// to track it
	int					itemID;			// whom the news in about (subject)
	int					secondItemID;	// secondary player
	U32					date;			// when it happened, in msec
	int					team;			// team of the primary item

	void Clear() {
		what = 0;
		pos.Zero();
		itemID = 0;
		secondItemID = 0;
		date = 0;
		team = 0;
	}
};


class NewsHistory
{
public:
	NewsHistory( ChitBag* chitBag );
	~NewsHistory();

	void Add( const NewsEvent& event );

	// First thing to tick! Updates the main clock.
	void DoTick( U32 delta );
	void Serialize( XStream* xs );

	// Returns on important events
	int NumNews() const { return events.Size(); }
	const NewsEvent& News( int i ) { GLASSERT( i >= 0 && i < events.Size() ); return events[i]; }

	// Returns all events
	enum { MAX_CURRENT = 20 };
	const grinliz::CArray< NewsEvent, MAX_CURRENT >& CurrentNews() const { return current; }

	struct Data {
		Data() : born(0), died(0) {}

		U32 born;
		U32 died;
	};
	const NewsEvent** Find( int itemID, bool includeSecond, int* num, Data* data );

private:
	// Time
	int    AgeI() const { return date / AGE_IN_MSEC; }
	double AgeD() const { return double(date) / double(AGE_IN_MSEC); }
	float  AgeF() const { return float(AgeD()); }

	U32 date;
	ChitBag* chitBag;
	grinliz::CDynArray< const NewsEvent* > cache;	// return from query call
	grinliz::CDynArray< NewsEvent > events;			// big array of everything that has happend.
	grinliz::CArray< NewsEvent, MAX_CURRENT > current;

};


#endif // ALTERA_NEWS_INCLUDED
