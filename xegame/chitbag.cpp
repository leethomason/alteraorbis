#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"

ChitBag::ChitBag()
{
}


ChitBag::~ChitBag()
{
	DeleteAll();
}


void ChitBag::DeleteAll()
{
	for( int i=0; i<chits.Size(); ++i ) {
		delete chits[i];
		chits[i] = 0;
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



Chit* ChitBag::CreateTestChit( Engine* engine, const char* assetName )
{
	Chit* chit = new Chit();
	chit->Add( new SpatialComponent() );
	chit->Add( new RenderComponent( engine, assetName ) );
	this->AddChit( chit );
	return chit;
}
