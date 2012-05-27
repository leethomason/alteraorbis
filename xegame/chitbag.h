#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glrectangle.h"

#include "../engine/enginelimits.h"
#include "../SimpleLib/SimpleLib.h"

#include "chit.h"
#include "xegamelimits.h"

class Engine;
	
// Shallow
struct ChitEvent
{
	ChitEvent() : id( 0 )	{}
	ChitEvent( int _id, int _data0=0 ) : id(_id), data0(_data0) { bounds.Zero(); }

	int id;
	int data0;
	grinliz::Rectangle2F bounds;
};


class ChitBag
{
public:
	ChitBag();
	~ChitBag();

	void DeleteAll();
	Chit* NewChit();
	void  DeleteChit( Chit* );
	Chit* GetChit( int id );

	// Calls every chit that has a tick.
	void DoTick( U32 delta );		

	// Due to events, changes, etc. a chit may need an update, possibily in addition to, the tick.
	// Normally called automatically.
	void RequestUpdate( Chit* );
	void QueueDelete( Chit* chit );

	void QueueEvent( const ChitEvent& event )			{ events.Push( event ); }
	const grinliz::CDynArray<ChitEvent>& GetEvents()	{ return events; }

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
	Simple::CIndex< int, Chit*, 
					Simple::SValue, Simple::SOwnedPtr> chits;	// Owned chit pointers. Note that CIndex 
																// supports fast iteration, which is critical.
	grinliz::CDynArray<Chit*> updateList;		// chits that need the Update() called. Can
												// have duplicates, which seems to be faster than
												// guarenteeing uniqueness.
	Simple::CSortedVector<Chit*> deleteList;					// <set> of chits that need to be deleted.
	grinliz::CDynArray<Chit*> hashQuery;
	grinliz::CDynArray<ChitEvent> events;
	Chit* spatialHash[SIZE*SIZE];
};


#endif // XENOENGINE_CHITBAG_INCLUDED