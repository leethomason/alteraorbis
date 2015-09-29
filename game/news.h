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
class Chit;
class GameItem;

// Lots of these - needs to be small. These are kept for the entire
// record of the game. 
class NewsEvent {
	friend class NewsHistory;
	
public:
	NewsEvent() { Clear(); }
	NewsEvent( U32 what, const grinliz::Vector2F& pos, int firstItem, int secondItem );

	void Serialize( XStream* xs );

public:
	// Be sure to update GetWhat() when this list chnages.
	// Be sure to update ToMessage()
	enum {							//	FIRST		SECOND
		NONE,
		DENIZEN_CREATED,			//	created
		DENIZEN_KILLED,				//  killed		killer
		GREATER_MOB_CREATED,		//  created				
		GREATER_MOB_KILLED,			//  killed		killer
		DOMAIN_CREATED,				//  domain
		DOMAIN_DESTROYED,			//  domain		killer	
		FORGED,						//	item		maker
		UN_FORGED,					//  item		killer

		DOMAIN_TAKEOVER,			//	domain		conqueror		// neutral takeover
		DOMAIN_CONQUER,				//	domain		conquerer		// sub-super
		SUPERTEAM_DELETED,			//  domain
		SUBTEAM_DELETED,			//  subTeam		superTeam
		ROGUE_DENIZEN_JOINS_TEAM,	//  denizen	

		PURCHASED,					//	purchaser	item
		STARVATION,					//  victim				
		BLOOD_RAGE,					//  victim				
		VISION_QUEST,				//	victim
		GREATER_SUMMON_TECH,		//  mob
		
		ATTITUDE_FRIEND,			//  origin		opinionOf
		ATTITUDE_NEUTRAL,
		ATTITUDE_ENEMY,

		PLOT_SWARM_START,			// ---
		PLOT_SWARM_END,

		NUM_WHAT
	};

	bool				Origin() const { return    what == DENIZEN_CREATED
												|| what == GREATER_MOB_CREATED
												|| what == DOMAIN_CREATED
												|| what == FORGED; }
	grinliz::IString	GetWhat() const;
	void				Console( grinliz::GLString* str, ChitBag*, int shortNameForThisID ) const;

	int What() const						{ return what; }
	const grinliz::Vector2F& Pos() const	{ return pos; }
	grinliz::Vector2I	Sector() const		{ return ToSector( ToWorld2I( pos )); }

	int	FirstItemID() const		{ return firstItemID; }
	int SecondItemID() const	{ return secondItemID; }

	int FirstTeam() const		{ return firstTeam; }
	int SecondTeam() const		{ return secondTeam; }

	static grinliz::IString IDToName( int id, bool shortName );

	static void Test();

private:
	int					what;	
	grinliz::Vector2F	pos;			// where it happened
	int					firstItemID;
	int					secondItemID;
	int					firstTeam;		// teams change; the team info needs to be stored separate from the item.
	int					secondTeam;
	U32					date;			// when it happened, in msec

	void Clear() {
		what = 0;
		pos.Zero();
		firstItemID = 0;
		secondItemID = 0;
		firstTeam = 0;
		secondTeam = 0;
		date = 0;
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

	struct Data {
		Data() : born(0), died(0) {}

		U32 born;
		U32 died;
	};
	const NewsEvent** FindItem( int itemID, int secondItemID, int* num, Data* data );

private:
	// Time
	int    AgeI() const { return date / AGE_IN_MSEC; }
	double AgeD() const { return double(date) / double(AGE_IN_MSEC); }
	float  AgeF() const { return float(AgeD()); }

	U32 date;
	ChitBag* chitBag;
	grinliz::CDynArray< const NewsEvent* > cache;	// return from query call
	grinliz::CDynArray< NewsEvent > events;			// big array of everything that has happend.

};


#endif // ALTERA_NEWS_INCLUDED
