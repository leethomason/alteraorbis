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

	// Hashes based on integer coordinates. No need to call
	// if they don't change.
	void AddToSpatialHash( Chit*, int x, int y );
	void RemoveFromSpatialHash( Chit*, int x, int y );
	void UpdateSpatialHash( Chit*, int x0, int y0, int x1, int y1 );
	const grinliz::CDynArray<Chit*, 32>& QuerySpatialHash( const grinliz::Rectangle2F& r, const Chit* ignoreMe );

private:
	enum {
		SHIFT = 2,	// a little tuning done; seems reasonable
		SIZE = MAX_MAX_SIZE >> SHIFT
	};
	U32 HashIndex( U32 x, U32 y ) const {
		return ( (y>>SHIFT)*SIZE + (x>>SHIFT) );
	}
	int idPool;
	Simple::CIndex< int, Chit*, 
					Simple::SValue, Simple::SOwnedPtr> chits;	// Owned chit pointers. Note that CIndex 
																// supports fast iteration, which is critical.
	grinliz::CDynArray<Chit*> updateList;		// chits that need the Update() called. Can
												// have duplicates, which seems to be faster than
												// guarenteeing uniqueness.
	Simple::CSortedVector<Chit*> deleteList;					// <set> of chits that need to be deleted.
	grinliz::CDynArray<Chit*, 32> hashQuery;
	Chit* spatialHash[SIZE*SIZE];
};


#endif // XENOENGINE_CHITBAG_INCLUDED