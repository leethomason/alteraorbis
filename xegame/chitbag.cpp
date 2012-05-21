#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "../grinliz/glperformance.h"

using namespace Simple;
using namespace grinliz;

ChitBag::ChitBag()
{
	idPool = 0;
	//updateList.Resort( 0, false );	// no dupes - turn into a set.
	deleteList.Resort( 0, false );
	memset( spatialHash, 0, sizeof(*spatialHash)*SIZE*SIZE );
}


ChitBag::~ChitBag()
{
	DeleteAll();
}


void ChitBag::DeleteAll()
{
	chits.RemoveAll();	// calles delete, since the list is set with "owned"
}


Chit* ChitBag::NewChit()
{
	Chit* chit = new Chit( ++idPool, this );
	chits.Add( chit->ID(), chit );
	return chit;
}


void ChitBag::DeleteChit( Chit* chit ) 
{
	GLASSERT( chits.Find( chit->ID(), chit ));
	chits.Remove( chit->ID() );
}


void ChitBag::RequestUpdate( Chit* chit )
{
	updateList.Push( chit );
}


void ChitBag::QueueDelete( Chit* chit )
{
	deleteList.Add( chit );
}


void ChitBag::DoTick( U32 delta )
{
	GRINLIZ_PERFTRACK;

	for( int i=0; i<chits.GetSize(); ++i ) {
		Chit* c = chits[i].Value;
		if ( c->NeedsTick() ) {
			c->DoTick( delta );
		}
	}
	while( !updateList.Empty() ) {
		Chit* c = updateList.Pop();
		c->DoUpdate();
	}
	for( int i=0; i<deleteList.GetSize(); ++i ) {
		//GLOUTPUT(( "ChitBag queude delete: %x\n", deleteList[i] ));
		DeleteChit( deleteList[i] );
	}
	deleteList.RemoveAll();
}


void ChitBag::AddToSpatialHash( Chit* chit, int x, int y )
{
	GLASSERT( chit );
	GLASSERT( chit->GetSpatialComponent() );

	U32 index = HashIndex( x, y );
	chit->next = spatialHash[index];
	spatialHash[index] = chit;
}


void ChitBag::RemoveFromSpatialHash( Chit* chit, int x, int y )
{
	GLASSERT( chit );
	GLASSERT( chit->GetSpatialComponent() );

	U32 index = HashIndex( x, y );

	Chit* prev = 0;
	for( Chit* it=spatialHash[index]; it; prev=it, it=it->next ) {
		if ( it == chit ) {
			if ( prev ) {
				prev->next = it->next;
				it->next = 0;
				return;
			}
			else {
				spatialHash[index] = it->next;
				it->next = 0;
				return;
			}
		}
	}
	GLASSERT( 0 );
}


void ChitBag::UpdateSpatialHash( Chit* c, int x0, int y0, int x1, int y1 )
{
	if ( (x0>>SHIFT)!=(x1>>SHIFT) || (y0>>SHIFT)!=(y1>>SHIFT) ) {
		RemoveFromSpatialHash( c, x0, y0 );
		AddToSpatialHash( c, x1, y1 );
	}
}


const CDynArray<Chit*,32>& ChitBag::QuerySpatialHash( const Rectangle2F& rf, const Chit* ignore )
{
	Rectangle2I r;

	r.Set( (int)rf.min.x, (int)rf.min.y, (int)ceilf(rf.max.x), (int)ceilf(rf.max.y) );
	r.min.x >>= SHIFT;
	r.min.y >>= SHIFT;
	r.max.x >>= SHIFT;
	r.max.y >>= SHIFT;

	hashQuery.Clear();
	for( int y=r.min.y; y<=r.max.y; ++y ) {
		for( int x=r.min.x; x<=r.max.x; ++x ) {
			unsigned index = y*SIZE+x;
			for( Chit* it=spatialHash[ index ]; it; it=it->next ) {
				if ( it != ignore ) {
					const Vector3F& pos = it->GetSpatialComponent()->GetPosition();
					if ( rf.Contains( pos.x, pos.z ) ) {
						hashQuery.Push( it );
					}
				}
			}
		}
	}
	return hashQuery;
}
