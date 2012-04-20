#include "chitbag.h"
#include "chit.h"

ChitBag::ChitBag()
{
}


ChitBag::~ChitBag()
{
	for( int i=0; i<chits.Size(); ++i ) {
		delete chits[i];
	}
}


void ChitBag::AddChit( Chit* chit )
{
	chits.Push( chit );
}


void ChitBag::RemoveChit( Chit* chit ) 
{
	// FIXME: not efficient, but not clear this is a needed function.
	for( int i=0; i<chits.Size(); ++i ) {
		if ( chits[i] == chit ) {
			chits.SwapRemove( i );
			return;
		}
	}
	GLASSERT( 0 );
}


void ChitBag::DoTick( U32 delta )
{
	for( int i=0; i<chits.Size(); ++i ) {
		// FIXME break into 2 lists
		if ( chits[i]->NeedsTick() ) {
			chits[i]->DoTick( delta );
		}
	}
}