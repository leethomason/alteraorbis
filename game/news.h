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
		LESSER_MOB_NAMED,			//	created
		LESSER_NAMED_MOB_KILLED,	//	killed		killer		killed

		FORGED,						//	item		maker		maker		
		UN_FORGED,					//  item		killer	
		SECTOR_HERD,				//	mob

		VOLCANO,
		POOL,
		WATERFALL,

		PURCHASED,					//	item		purchaser	0

		NUM_WHAT
	};

	bool				Origin() const { return    what == DENIZEN_CREATED
												|| what == GREATER_MOB_CREATED
												|| what == LESSER_MOB_NAMED
												|| what == FORGED; }
	grinliz::IString	GetWhat() const;
	void				Console( grinliz::GLString* str ) const;
	grinliz::Vector2I	Sector() const { return ToSector( ToWorld2I( pos )); }
	grinliz::IString	IDToName( int id ) const;

	int					what;	
	grinliz::Vector2F	pos;			// where it happened
	// Use Items instead of Chits so we can look up history.
	int					chitID;			// to track it
	int					itemID;			// whom the news in about (subject)
	int					secondItemID;	// secondary player
	U32					date;			// when it happened, in msec

	void Clear() {
		what = 0;
		pos.Zero();
		itemID = 0;
		secondItemID = 0;
		date = 0;
	}
};


// "scoped singleton": created and destroyed by a main game/scene scope, 
// but there can only be one active. (Could implement a stack, in the future.)
// Created and destroyed by the Simulation
class NewsHistory : public StackedSingleton< NewsHistory >
{
public:
	NewsHistory( ChitBag* chitBag );
	~NewsHistory();

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

	const NewsEvent** Find( int itemID, bool includeSecond, int* num );

	ChitBag* GetChitBag() const { return chitBag; }

private:

	U32 date;
	ChitBag* chitBag;
	grinliz::CDynArray< const NewsEvent* > cache;	// return from query call
	grinliz::CDynArray< NewsEvent > events;			// big array of everything that has happend.
};


#endif // ALTERA_NEWS_INCLUDED
