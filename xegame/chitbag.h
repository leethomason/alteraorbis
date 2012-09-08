#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"

#include "../engine/enginelimits.h"
#include "../engine/bolt.h"

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

	// Bolts are a special kind of chit. Just easier
	// and faster to treat them as a 2nd stage.
	Bolt* NewBolt();
	const Bolt* BoltMem() const { return bolts.Mem(); }
	int NumBolts() const { return bolts.Size(); }

	// Calls every chit that has a tick.
	void DoTick( U32 delta, Engine* engine );	
	U32 AbsTime() const { return bagTime; }

	int NumChits() const { return chitID.NumValues(); }
	int NumTicked() const { return nTicked; }

	// Due to events, changes, etc. a chit may need an update, possibily in addition to, the tick.
	// Normally called automatically.
	void QueueDelete( Chit* chit );

	// passes ownership
	void QueueEvent( ChitEvent* event )			{ events.Push( event ); }

	// Hashes based on integer coordinates. No need to call
	// if they don't change.
	void AddToSpatialHash( Chit*, int x, int y );
	void RemoveFromSpatialHash( Chit*, int x, int y );
	void UpdateSpatialHash( Chit*, int x0, int y0, int x1, int y1 );

	void QuerySpatialHash(	grinliz::CDynArray<Chit*>* array, 
							const grinliz::Rectangle2F& r, 
		                    const Chit* ignoreMe,
							int itemFilter );

private:
	enum {
		SHIFT = 2,	// a little tuning done; seems reasonable
		SIZE = MAX_MAP_SIZE >> SHIFT
	};
	U32 HashIndex( U32 x, U32 y ) const {
		return ( (y>>SHIFT)*SIZE + (x>>SHIFT) );
	}

	int idPool;
	U32 bagTime;
	int nTicked;

	enum { BLOCK_SIZE = 1000 };
	Chit* memRoot;
	// Use a memory pool so the chits don't move on re-allocation.
	grinliz::CDynArray< Chit* >		  blocks;
	grinliz::HashTable< int, Chit* >  chitID;

	grinliz::CDynArray<int>			deleteList;		// <set> of chits that need to be deleted.
	grinliz::CDynArray<Chit*>		hashQuery;
	grinliz::CDynArray<ChitEvent*, grinliz::OwnedPtrSem >	events;
	
	grinliz::CDynArray<Bolt> bolts;
	
	Chit* spatialHash[SIZE*SIZE];
};


#endif // XENOENGINE_CHITBAG_INCLUDED