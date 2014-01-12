/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"

#include "../engine/enginelimits.h"
#include "../engine/bolt.h"

#include "chit.h"
#include "xegamelimits.h"
#include "../game/news.h"
#include "chitevent.h"

class Engine;
class ComponentFactory;
class XStream;
class CameraComponent;

#define CChitArray grinliz::CArray<Chit*, 32 >

// Controls tuning of spatial hash. Helps, but 
// still spending way too much time finding out 
// what is around a given chit.
#define SPATIAL_VAR

// This ticks per-component instead of per-chit.
// Rather expected this to make a difference
// (cache use) but doesn't.
//#define OUTER_TICK

class IChitAccept
{
public:
	enum {
		MAP = 0x01,
		MOB = 0x02
	};
	virtual bool Accept( Chit* chit ) = 0;
	virtual int  Type() = 0;
};


class ChitAcceptAll : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit )	{ return true; }
	virtual int  Type()					{ return MAP | MOB; }
};

class ChitHasMoveComponent : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type()					{ return MOB; }
};

class ChitHasAIComponent : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type()					{ return MAP | MOB; }
};

class MultiFilter : public IChitAccept
{
public:
	enum {
		MATCH_ANY,
		MATCH_ALL
	};
	MultiFilter( int anyAllMatch ) : type(-1), anyAll(anyAllMatch) {}

	virtual bool Accept( Chit* chit );
	virtual int  Type();					// committed when called the first time

	grinliz::CDynArray< IChitAccept* > filters;
private:
	int type;
	int anyAll;
};


class ChitBag : public IBoltImpactHandler
{
public:
	ChitBag();
	virtual ~ChitBag();

	// Chit creation/query
	void DeleteAll();
	Chit* NewChit( int id=0 );
	void  DeleteChit( Chit* );
	Chit* GetChit( int id );

	void Serialize( const ComponentFactory* factory, XStream* xs );

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
	void QueueRemoveAndDeleteComponent( Component* comp );
	void DeferredDelete( Component* comp );

	// passes ownership
	void QueueEvent( const ChitEvent& event )			{ events.Push( event ); }

	// Hashes based on integer coordinates. No need to call
	// if they don't change.
	void AddToSpatialHash( Chit*, int x, int y );
	void RemoveFromSpatialHash( Chit*, int x, int y );
	void UpdateSpatialHash( Chit*, int x0, int y0, int x1, int y1 );

	void QuerySpatialHash(	grinliz::CDynArray<Chit*>* array, 
							const grinliz::Rectangle2F& r, 
		                    const Chit* ignoreMe,
							IChitAccept* filter );

	void QuerySpatialHash(	CChitArray* arr,
							const grinliz::Rectangle2F& r, 
							const Chit* ignoreMe,
							IChitAccept* filter );

	void QuerySpatialHash(	grinliz::CDynArray<Chit*>* array,
							const grinliz::Vector2F& origin, 
							float rad,
							const Chit* ignoreMe,
							IChitAccept* accept )
	{
		grinliz::Rectangle2F r;
		r.Set( origin.x-rad, origin.y-rad, origin.x+rad, origin.y+rad );
		QuerySpatialHash( array, r, ignoreMe, accept );
	}

	void QuerySpatialHash(	CChitArray* array,
							const grinliz::Vector2F& origin, 
							float rad,
							const Chit* ignoreMe,
							IChitAccept* accept )
	{
		grinliz::Rectangle2F r;
		r.Set( origin.x-rad, origin.y-rad, origin.x+rad, origin.y+rad );
		QuerySpatialHash( array, r, ignoreMe, accept );
	}

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	// There can only be one camera actually in use:
	CameraComponent* GetCamera( Engine* engine );
	int GetCameraChitID() const { return activeCamera; }

	virtual LumosChitBag* ToLumos() { return 0; }

private:

	enum {
		SIZE2 = 1024*64	// 16 bits
	};

	U32 HashIndex( U32 x, U32 y ) const {
		//return (y*MAX_MAP_SIZE + x) & (SIZE2-1);
		//return (x ^ (y<<6)) & (SIZE2-1);				// 16 bit variation
		return (y*137 +x ) & (SIZE2-1);					// prime # wins the day.
	}

	int idPool;
	U32 bagTime;
	int nTicked;
	int activeCamera;

	struct CompID { 
		int chitID;
		int compID;
	};

	enum { BLOCK_SIZE = 8*1000 };
	Chit* memRoot;
	// Use a memory pool so the chits don't move on re-allocation. I
	// keep wanting to use a DynArray, but that opens up issues of
	// Chit copying (which they aren't set up for.) It keeps going 
	// squirrely. If inclined, I think a custom array based on realloc
	// is probably the correct solution, but the current approach 
	// may be fine.
	grinliz::CDynArray< Chit* >		  blocks;
	grinliz::HashTable< int, Chit* >  chitID;
	grinliz::CDynArray<int>			deleteList;	
	grinliz::CDynArray<CompID>		compDeleteList;		// delete and remove list
	grinliz::CDynArray<Component*>	zombieDeleteList;	// just delete
	grinliz::CDynArray<Chit*>		hashQuery;			// local data, cached at class level
	grinliz::CDynArray<Chit*>		cachedQuery;		// local data, cached at class level
	grinliz::CDynArray<ChitEvent>	events;
	grinliz::CDynArray<Bolt>		bolts;
#ifdef OUTER_TICK
	grinliz::CDynArray<Component*>	tickList[Chit::NUM_SLOTS];
#endif
	
	// Split out into 2 hashes for performance, and
	// independent tweaking.
	Chit* mapSpatialHash[SIZE2];
	Chit* mobSpatialHash[SIZE2];
};


#endif // XENOENGINE_CHITBAG_INCLUDED