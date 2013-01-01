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

#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"

#include "../engine/model.h"
#include "../grinliz/glperformance.h"
#include "../tinyxml2/tinyxml2.h"

using namespace grinliz;
using namespace tinyxml2;

ChitBag::ChitBag()
{
	idPool = 0;
	bagTime = 0;
	memset( spatialHash, 0, sizeof(*spatialHash)*SIZE*SIZE );
	memRoot = 0;
}


ChitBag::~ChitBag()
{
	DeleteAll();
}


void ChitBag::DeleteAll()
{
	chitID.RemoveAll();
	while ( !blocks.Empty() ) {
		Chit* block = blocks.Pop();
		delete [] block;
	}
	bolts.Clear();
}


void ChitBag::Save( const char* pathToXML )
{
	FILE* fp = fopen( pathToXML, "w" );
	GLASSERT( fp );
	if ( fp ) {
		XMLPrinter printer( fp );

		printer.OpenElement( "ChitBag" );
		printer.PushAttribute( "idPool", idPool );
		printer.PushAttribute( "bagTime", bagTime );

		printer.OpenElement( "Chits" );
		Chit** chits = chitID.GetValues();
		for( int i=0; i<chitID.NumValues(); ++i ) {
			chits[i]->Save( &printer );
		}
		printer.CloseElement();	// Chits

		printer.CloseElement();	// ChitBag
	}
}


Chit* ChitBag::NewChit()
{
	if ( !memRoot ) {
		// Allocate a new block.
		Chit* block = new Chit[BLOCK_SIZE];

		// Hook up the free memory pointers.
		block[BLOCK_SIZE-1].next = 0;
		for( int i=BLOCK_SIZE-2; i>=0; --i ) {
			block[i].next = &block[i+1];
		}
		memRoot = &block[0];
		blocks.Push( block );
	}
	Chit* c = memRoot;
	memRoot = memRoot->next;
	c->next = 0;
	c->Init( ++idPool, this );
	chitID.Add( c->ID(), c );
	return c;
}


void ChitBag::DeleteChit( Chit* chit ) 
{
	GLASSERT( chitID.Query( chit->ID(), 0 ));
	chitID.Remove( chit->ID() );
	chit->Free();
	// Link back in to free pool.
	chit->next = memRoot;
	memRoot = chit;
}


Chit* ChitBag::GetChit( int id )
{
	Chit* c = 0;
	chitID.Query( id, &c );
	return c;
}


void ChitBag::QueueDelete( Chit* chit )
{
	deleteList.Push( chit->ID() );
}


void ChitBag::QueueDeleteComponent( Component* comp )
{
	CompID* c = compDeleteList.PushArr(1);
	c->chitID = comp->ParentChit()->ID();
	c->compID = comp->ID();
}


Bolt* ChitBag::NewBolt()
{
	return bolts.PushArr(1);
}


void ChitBag::DoTick( U32 delta, Engine* engine )
{
	GRINLIZ_PERFTRACK;
	bagTime += delta;
	nTicked = 0;

	// Events.
	// Ticks.
	// Bolts.
	// probably correct in that order.

	// Process in order, and remember event queue can mutate as we go.
	while ( !events.Empty() ) {
		ChitEvent e = events[0];
		events.Remove( 0 );

		QuerySpatialHash( &hashQuery, e.AreaOfEffect(), 0, e.ItemFilter() );
		for( int j=0; j<hashQuery.Size(); ++j ) {
			hashQuery[j]->OnChitEvent( e );
		}
	}

	for( int i=0; i<blocks.Size(); ++i ) {
		Chit* block = blocks[i];
		for( int j=0; j<BLOCK_SIZE; ++j ) {
			if ( block[j].ID() && block[j].TickNeeded() ) {
				++nTicked;
				block[j].DoTick( delta );
			}
		}
	}
	for( int i=0; i<deleteList.Size(); ++i ) {
		Chit* chit = 0;
		chitID.Query( deleteList[i], &chit );
		if ( chit ) {
			DeleteChit( chit );
		}
	}
	deleteList.Clear();


	for( int i=0; i<compDeleteList.Size(); ++i ) {
		Chit* chit = 0;
		chitID.Query( compDeleteList[i].chitID, &chit );
		if ( chit ) {
			Component* c = chit->GetComponent( compDeleteList[i].compID );
			if ( c ) {
				chit->Remove( c );
				delete c;
			}
		}
	}
	compDeleteList.Clear();


	if ( engine ) {
		Bolt::TickAll( &bolts, delta, engine, this );
	}
}


void ChitBag::HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at )
{
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


/*
Vector3F ChitBag::compareOrigin;

int ChitBag::CompareDistance( const void* p0, const void* p1 ) 
{
	Chit** c0 = (Chit**)p0;
	Chit** c1 = (Chit**)p1;

	float d0 = ( (*c0)->GetSpatialComponent()->GetPosition() - compareOrigin).LengthSquared();
	float d1 = ( (*c1)->GetSpatialComponent()->GetPosition() - compareOrigin).LengthSquared();

	return LRintf(d0 - d1);
}
*/

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
						if ( it->GetItemComponent() && (it->GetItemComponent()->GetItem()->flags & itemFilter )) {
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
}
