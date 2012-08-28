#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "../grinliz/glperformance.h"

using namespace grinliz;

ChitBag::ChitBag()
{
	idPool = 0;
	bagTime = 0;
	memset( spatialHash, 0, sizeof(*spatialHash)*SIZE*SIZE );
}


ChitBag::~ChitBag()
{
	DeleteAll();
}


void ChitBag::DeleteAll()
{
	chits.RemoveAll();
}


Chit* ChitBag::NewChit()
{
	Chit* chit = new Chit( ++idPool, this );
	chits.Add( chit->ID(), chit );
	return chit;
}


void ChitBag::DeleteChit( Chit* chit ) 
{
	GLASSERT( chits.Query( chit->ID(), 0 ));
	chits.Remove( chit->ID() );
}


Chit* ChitBag::GetChit( int id )
{
	Chit* c = 0;
	chits.Query( id, &c );
	return c;
}


void ChitBag::QueueDelete( Chit* chit )
{
	deleteList.Push( chit->ID() );
}


void ChitBag::DoTick( U32 delta )
{
	GRINLIZ_PERFTRACK;
	bagTime += delta;
	nTicked = 0;

	// Events.
	// Ticks.
	// probably correct in that order.

	for( int i=0; i<events.Size(); ++i ) {
		const ChitEvent& e = *events[i];
		QuerySpatialHash( &hashQuery, e.AreaOfEffect(), 0, e.GetItemFilter() );
		for( int j=0; j<hashQuery.Size(); ++j ) {
			hashQuery[j]->OnChitEvent( e );
		}
	}

	Chit** chitArr = chits.GetValues();
	for( int i=0; i<chits.NumValues(); ++i ) {
		Chit* c = chitArr[i];
		if ( c->TickNeeded() ) {
			++nTicked;
			c->DoTick( delta );
		}
	}
	for( int i=0; i<deleteList.Size(); ++i ) {
		Chit* chit = 0;
		chits.Query( deleteList[i], &chit );
		if ( chit ) {
			DeleteChit( chit );
		}
	}
	deleteList.Clear();
	events.Clear();
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


Vector3F ChitBag::compareOrigin;

int ChitBag::CompareDistance( const void* p0, const void* p1 ) 
{
	Chit** c0 = (Chit**)p0;
	Chit** c1 = (Chit**)p1;

	float d0 = ( (*c0)->GetSpatialComponent()->GetPosition() - compareOrigin).LengthSquared();
	float d1 = ( (*c1)->GetSpatialComponent()->GetPosition() - compareOrigin).LengthSquared();

	return LRintf(d0 - d1);
}


void ChitBag::QuerySpatialHash(	grinliz::CDynArray<Chit*>* array, 
								const grinliz::Rectangle2F& rf, 
								const Chit* ignore,
								int itemFilter )
{
	Rectangle2I b;
	b.Set( 0, 0, SIZE-1, SIZE-1 );

	Rectangle2I r;

	r.Set( (int)rf.min.x, (int)rf.min.y, (int)ceilf(rf.max.x), (int)ceilf(rf.max.y) );
	r.min.x >>= SHIFT;
	r.min.y >>= SHIFT;
	r.max.x >>= SHIFT;
	r.max.y >>= SHIFT;
	r.DoIntersection( b );

	array->Clear();
	for( int y=r.min.y; y<=r.max.y; ++y ) {
		for( int x=r.min.x; x<=r.max.x; ++x ) {
			unsigned index = y*SIZE+x;
			for( Chit* it=spatialHash[ index ]; it; it=it->next ) {
				if ( it != ignore ) {
					if ( itemFilter ) {
						if ( it->GetItemComponent() && (it->GetItemComponent()->item.flags & itemFilter )) {
							// okay!
						}
						else {
							continue;
						}
					}
					const Vector3F& pos = it->GetSpatialComponent()->GetPosition();
					if ( rf.Contains( pos.x, pos.z ) ) {
						array->Push( it );
					}
				}
			}
		}
	}
	/*
	if ( array->Size() > 1 && distanceSort ) {
		Vector2F origin = rf.Center();
		compareOrigin.Set( origin.x, 0, origin.y );
		qsort( array->Mem(), array->Size(), sizeof(Chit*), CompareDistance ); 
		for( int i=0; i<array->Size(); ++i ) {
			GLOUTPUT(( "sh: %d d=%.1f\n", (*array)[i]->ID(), ((*array)[i]->GetSpatialComponent()->GetPosition() - compareOrigin).Length() ));
		}
	}
	*/
}
