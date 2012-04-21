#include "chit.h"
#include "chitbag.h"
#include "component.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"


Chit::Chit() :	chitBag( 0 ), nTickers( 0 )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		slot[i] = 0;
	}
}


Chit::~Chit()
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			slot[i]->OnRemove();
			delete slot[i];
		}
	}
}

	
void Chit::Add( Component* c )
{
	if ( c->ToSpatial() ) {
		GLASSERT( !slot[SPATIAL] );
		slot[SPATIAL] = c;
		GLASSERT( spatialComponent == c );
	}
	if ( c->ToRender() ) {
		GLASSERT( !slot[RENDER] );
		slot[RENDER] = c;
		GLASSERT( renderComponent == c );
	}
	c->OnAdd( this );
	if ( c->NeedsTick() )
		++nTickers;
}


void Chit::Remove( Component* c )
{
	if ( c->NeedsTick() )
		--nTickers;

	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] == c ) {
			c->OnRemove();
			slot[i] = 0;
			return;
		}
	}
	GLASSERT( 0 );	// not found
}


void Chit::RequestUpdate()
{
	if ( chitBag ) {
		chitBag->RequestUpdate( this );
	}
}



void Chit::DoTick( U32 delta )
{
	GLASSERT( NeedsTick() );
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) 
			slot[i]->DoTick( delta );
	}
}


void Chit::DoUpdate()
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) 
			slot[i]->DoUpdate();
	}
}




