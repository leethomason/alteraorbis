#include "chit.h"
#include "chitbag.h"
#include "component.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"

#include "../grinliz/glstringutil.h"

using namespace grinliz;
const U32 SLOW_TICK = 500;

Chit::Chit( int _id, ChitBag* bag ) :	chitBag( bag ), id( _id ), nTickers( 0 ), oneTickNeeded( true )
{
	next = 0;
	for( int i=0; i<NUM_SLOTS; ++i ) {
		slot[i] = 0;
	}
	slowTickTimer = (_id * 37)%SLOW_TICK;
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
	int s = -1;
	if ( c->ToComponent( "SpatialComponent" )) {
		s = SPATIAL;
	}
	else if ( c->ToComponent( "MoveComponent" )) {
		s = MOVE;
	}
	else if ( c->ToComponent( "InventoryComponent" )) {
		s = INVENTORY;
	}
	else if ( c->ToComponent( "ItemComponent" )) {
		s = ITEM;
	}
	else if ( c->ToComponent( "RenderComponent" )) {
		s = RENDER;
	}

	if ( s >= 0 ) {
		GLASSERT( !slot[s] );
		slot[s] = c;
	}
	else {
		int i;
		for( i=GENERAL_START; i<GENERAL_END; ++i ) {
			if ( slot[i] == 0 ) {
				slot[i] = c;
				break;
			}
		}
		GLASSERT( i < GENERAL_END );
	}
	c->OnAdd( this );
	if ( c->NeedsTick() ) {
		++nTickers;
	}
	oneTickNeeded = true;
}


void Chit::Remove( Component* c )
{
	if ( c->NeedsTick() )
		--nTickers;
	oneTickNeeded = true;

	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] == c ) {
			c->OnRemove();
			slot[i] = 0;
			return;
		}
	}
	GLASSERT( 0 );	// not found
}


Component* Chit::GetComponent( const char* name )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && slot[i]->ToComponent( name )) {
			return slot[i];
		}
	}
	return 0;
}


void Chit::DoTick( U32 delta )
{
	GLASSERT( NeedsTick() );
	oneTickNeeded = false;	// clear before tick, which may set the flag again
	slowTickTimer += delta;

	if ( slowTickTimer > SLOW_TICK ) {
		slowTickTimer = 0;
		for( int i=0; i<NUM_SLOTS; ++i ) {
			if ( slot[i] ) 
				slot[i]->DoSlowTick();
		}
	}
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) 
			slot[i]->DoTick( delta );
	}
}


void Chit::OnChitEvent( const ChitEvent& event )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			slot[i]->OnChitEvent( event );
		}
	}
}


void Chit::SendMessage( const ChitMsg& msg, Component* exclude )
{
	// Listeners.
	for( int i=0; i<listeners.Size(); ++i ) {
		listeners[i]->OnChitMsg( this, msg );
	}

	int i=0;
	while ( i<cListeners.Size() ) {
		Chit* target = GetChitBag()->GetChit( cListeners[i].chitID );
		if ( target == 0 ) {
			cListeners.SwapRemove(i);
			continue;
		}
		bool okay = target->CarryMsg( cListeners[i].componentID, this, msg );
		if ( !okay ) {
			// dead link
			cListeners.SwapRemove(i);
			continue;
		}
		++i;
	}

	// Components
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && slot[i] != exclude ) {
			slot[i]->OnChitMsg( this, msg );
		}
	}
}


bool Chit::CarryMsg( int componentID, Chit* src, const ChitMsg& msg )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && slot[i]->ID() == componentID ) {
			slot[i]->OnChitMsg( src, msg );
			return true;
		}
	}
	return false;
}


void Chit::AddListener( IChitListener* listener )
{
	GLASSERT( listeners.Find( listener ) < 0 );
	listeners.Push( listener );
}


void Chit::RemoveListener( IChitListener* listener )
{
	int i = listeners.Find( listener );
	GLASSERT( i >= 0 );
	listeners.SwapRemove( i );
}


void Chit::AddListener( Component* c )
{
	CList data;
	data.chitID = c->ParentChit()->ID();
	data.componentID = c->ID();

	cListeners.Push( data );
}


void Chit::RemoveListener( Component* c ) 
{
	CList data;
	data.chitID = c->ParentChit()->ID();
	data.componentID = c->ID();

	for( int i=0; i<cListeners.Size(); ++i ) {
		if ( cListeners[i].chitID == data.chitID && cListeners[i].componentID == data.componentID ) {
			cListeners.SwapRemove( i );
			return;
		}
	}
	GLASSERT( 0 );
}


void Chit::DebugStr( GLString* str )
{
	str->Format( "Chit=%x ", this );

	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			unsigned size = str->size();
			slot[i]->DebugStr( str );
			if ( size == str->size() ) {
				str->Format( "[] " );
			}
		}
	}
}


void SafeChitList::Init( ChitBag* cb )
{
	GLASSERT( array.Empty() );
	GLASSERT( chitBag == 0 );
	chitBag = cb;
}


void SafeChitList::Free() 
{
	array.Clear();
	chitBag = 0;
}


Chit* SafeChitList::Add( Chit* c )			
{
	int index = array.Find( c->ID() );
	if ( index < 0 ) {
		array.Push( c->ID() );
		return c;
	}
	return 0;
}


Chit* SafeChitList::Remove( Chit* c )
{
	int index = array.Find( c->ID() );
	GLASSERT( index >= 0 );
	if ( index > 0 ) {
		array.SwapRemove( index );
		return c;
	}
	return 0;
}


Chit* SafeChitList::First() const { 
	it = array.Size();
	return Next();
}


Chit* SafeChitList::Next() const {
	while ( it > 0 ) {
		--it;
		Chit* c = chitBag->GetChit( array[it] );
		if ( c == 0 ) {
			array.SwapRemove( it );
			--it;
		}
		return c;
	}
	return 0;
}


ComponentSet::ComponentSet( Chit* _chit, int bits )
{
	Zero();
	if ( _chit ) {
		chit = _chit;
		int error = 0;
		if ( bits & Chit::SPATIAL_BIT ) {
			spatial = chit->GetSpatialComponent();
			if ( !spatial ) ++error;
		}
		if ( bits & Chit::MOVE_BIT ) {
			move = chit->GetMoveComponent();
			if ( !move ) ++error;
		}
		if ( bits & Chit::INVENTORY_BIT ) {
			inventory = chit->GetInventoryComponent();
			if ( !inventory ) ++error;
		}
		if ( bits & Chit::ITEM_BIT ) {
			itemComponent = chit->GetItemComponent();
			if ( !itemComponent ) 
				++error;
			else
				item = &itemComponent->item;
		}
		{
			// Always check if alive:
			ItemComponent* ic = chit->GetItemComponent();
			if ( !ic ) alive = true;	// weird has-no-health-must-be-alive case
			if ( ic ) {
				alive = ic->GetItem()->hp > 0;
			}
			// But only set error if requested
			if ( bits & ComponentSet::IS_ALIVE ) {
				if ( !alive )
					++error;
			}
		}
		if ( bits & Chit::RENDER_BIT ) {
			render = chit->GetRenderComponent();
			if ( !render ) ++error;
		}

		if ( error ) 
			Zero();
		okay = !error;
	}
}


void ComponentSet::Zero()
{
	okay = false;
	chit = 0;
	spatial = 0;
	move = 0;
	inventory = 0;
	itemComponent = 0;
	item = 0;
	render = 0;
}


