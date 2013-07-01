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

#define WINDOWS_LEAN_AND_MEAN
#include <crtdbg.h>

#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "cameracomponent.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../grinliz/glperformance.h"
#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

ChitBag::ChitBag()
{
	idPool = 0;
	bagTime = 0;
	memset( spatialHash, 0, sizeof(*spatialHash)*SIZE*SIZE );
	memRoot = 0;
	activeCamera = 0;
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


void ChitBag::Serialize( const ComponentFactory* factory, XStream* xs )
{
	XarcOpen( xs, "ChitBag" );
	XARC_SER( xs, activeCamera );

	if ( xs->Saving() ) {
		XarcOpen( xs, "Chits" );
		Chit** chits = chitID.GetValues();
		for( int i=0; i<chitID.NumValues(); ++i ) {
			XarcOpen( xs, "id" );
			xs->Saving()->Set( "id", chits[i]->ID() );
			XarcClose( xs );
			chits[i]->Serialize( factory, xs  );
		}
		XarcClose( xs );

		XarcOpen( xs, "Bolts" );
		for( int i=0; i<bolts.Size(); ++i ) {
			bolts[i].Serialize( xs );
		}
		XarcClose( xs );
	}
	else {
		idPool = 0;

		XarcOpen( xs, "Chits" );
		while( xs->Loading()->HasChild() ) {

			int id = 0;
			XarcOpen( xs, "id" );
			XARC_SER_KEY( xs, "id", id );	
			XarcClose( xs );
			idPool = Max( id, idPool );

			//GLOUTPUT(( "loading chit id=%d\n", id ));
			Chit* c = this->NewChit( id );
			c->Serialize( factory, xs );
		}
		XarcClose( xs );

		XarcOpen( xs, "Bolts" );
		while( xs->Loading()->HasChild() ) {
			Bolt* b = bolts.PushArr( 1 );
			b->Serialize( xs );			
		}
		XarcClose( xs );
	}
	XarcClose( xs );
}


Chit* ChitBag::NewChit( int id )
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
	c->Init( id ? id : (++idPool), this );
	GLASSERT( chitID.Query( c->ID(), 0 ) == false );

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


void ChitBag::QueueRemoveAndDeleteComponent( Component* comp )
{
	CompID* c = compDeleteList.PushArr(1);
	c->chitID = comp->ParentChit()->ID();
	c->compID = comp->ID();
}


void ChitBag::DeferredDelete( Component* comp )
{
	GLASSERT( comp->ParentChit() == 0 );	// already removed.
	zombieDeleteList.Push( comp );
}


Bolt* ChitBag::NewBolt()
{
	Bolt bolt;
	Bolt* b = bolts.PushArr(1);
	*b = bolt;
	return b;
}


CameraComponent* ChitBag::GetCamera( Engine* engine )
{
	Chit* c = GetChit( activeCamera );
	if( c ) {
		CameraComponent* cc = GET_GENERAL_COMPONENT( c, CameraComponent );
		if ( cc )
			return cc;
	}
	if ( c ) { 
		c->QueueDelete(); 
	}
	c = NewChit();
	activeCamera = c->ID();
	CameraComponent* cc = new CameraComponent( &engine->camera );
	c->Add( cc );
	return cc;
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
		ChitEvent e = events.Pop();

		QuerySpatialHash( &hashQuery, e.AreaOfEffect(), 0 );
		for( int j=0; j<hashQuery.Size(); ++j ) {
			hashQuery[j]->OnChitEvent( e );
		}
	}

	for( int i=0; i<blocks.Size(); ++i ) {
		Chit* block = blocks[i];
		for( int j=0; j<BLOCK_SIZE; ++j ) {
			Chit* c = block + j;
			int id = c->ID();
			if ( id && id != activeCamera ) {
				
				c->timeToTick -= delta;
				c->timeSince  += delta;

				if ( c->timeToTick <= 0 ) {
					++nTicked;
					//_CrtCheckMemory();
					c->DoTick( delta );
					GLASSERT( c->timeToTick >= 0 );
					//_CrtCheckMemory();
				}
			}
		}
	}
	// Make sure the camera is updated last so it doesn't "drag"
	// what it's tracking. The next time this happens I should
	// put in a priority system.
	Chit* camera = GetChit( activeCamera );
	if ( camera ) {
		camera->DoTick( delta );
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

	while ( !zombieDeleteList.Empty() ) {
		Component* c = zombieDeleteList.Pop();
		GLASSERT( c->ParentChit() == 0 );
		delete c;
	}

	if ( engine ) {
		Bolt::TickAll( &bolts, delta, engine, this );
	}

	for( int i=0; i<news.Size(); ++i ) {
		news[i].age += delta;
	}
}


void ChitBag::HandleBolt( const Bolt& bolt, const ModelVoxel& mv )
{
}


void ChitBag::AddNews( const NewsEvent& event )
{
	news.PushFront( event );
	news[0].age = 0;

	if ( news.Size() > 40 ) {
		news.Pop();
	}
}


void ChitBag::SetNewsProcessed()
{
	for( int i=0; i<news.Size(); ++i ) {
		news[i].processed = true;
	}
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



bool ChitBag::HasMoveComponentFilter( Chit* chit )
{
	return chit->GetMoveComponent() != 0;
}


bool ChitBag::HasAIComponentFilter( Chit* chit )
{
	return chit->GetAIComponent() != 0;
}



void ChitBag::QuerySpatialHash(	grinliz::CDynArray<Chit*>* array, 
								const grinliz::Rectangle2F& rf, 
								const Chit* ignore,
								bool (*accept)(Chit*) )
{
	if ( !accept ) accept = AcceptAll;

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
					const Vector3F& pos = it->GetSpatialComponent()->GetPosition();
					if ( rf.Contains( pos.x, pos.z ) && accept( it )) {
						array->Push( it );
					}
				}
			}
		}
	}
}


void ChitBag::QuerySpatialHash(	CChitArray* arr,
								const grinliz::Rectangle2F& r, 
								const Chit* ignoreMe,
								bool (*accept)(Chit*) )
{
	QuerySpatialHash( &cachedQuery, r, ignoreMe, accept );
	arr->Clear();
	for( int i=0; i<cachedQuery.Size(); ++i ) {
		if ( !arr->HasCap() )
			break;
		arr->Push( cachedQuery[i] );
	}
}
