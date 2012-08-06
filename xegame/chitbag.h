#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"

#include "../engine/enginelimits.h"

#include "chit.h"
#include "xegamelimits.h"
#include "chitevent.h"

class Engine;
	

class ChitBag
{
public:
	ChitBag();
	~ChitBag();

	// Chit creation/query
	void DeleteAll();
	Chit* NewChit();
	void  DeleteChit( Chit* );
	Chit* GetChit( int id );

	// Calls every chit that has a tick.
	void DoTick( U32 delta );	
	U32 AbsTime() const { return bagTime; }

	// Due to events, changes, etc. a chit may need an update, possibily in addition to, the tick.
	// Normally called automatically.
	void QueueDelete( Chit* chit );

	void QueueEvent( const ChitEvent& event )			{ events.Push( event ); }

	// Hashes based on integer coordinates. No need to call
	// if they don't change.
	void AddToSpatialHash( Chit*, int x, int y );
	void RemoveFromSpatialHash( Chit*, int x, int y );
	void UpdateSpatialHash( Chit*, int x0, int y0, int x1, int y1 );
	const grinliz::CDynArray<Chit*>& QuerySpatialHash( const grinliz::Rectangle2F& r, 
		                                               const Chit* ignoreMe,
													   bool distanceSort );

private:
	enum {
		SHIFT = 2,	// a little tuning done; seems reasonable
		SIZE = MAX_MAP_SIZE >> SHIFT
	};
	U32 HashIndex( U32 x, U32 y ) const {
		return ( (y>>SHIFT)*SIZE + (x>>SHIFT) );
	}

	static grinliz::Vector3F compareOrigin;
	static int CompareDistance( const void* p0, const void* p1 );

	int idPool;
	U32 bagTime;
	// FIXME: chits could be stored in a DynArr. Go cohenent memory, go! Makes 
	// walking the high level list potentially faster.
	grinliz::HashTable< int, Chit*, grinliz::CompValue, grinliz::OwnedPtrSem > chits;
	grinliz::CDynArray<int>			deleteList;		// <set> of chits that need to be deleted.
	grinliz::CDynArray<Chit*>		hashQuery;
	grinliz::CDynArray<ChitEvent>	events;
	Chit* spatialHash[SIZE*SIZE];
};


#endif // XENOENGINE_CHITBAG_INCLUDED