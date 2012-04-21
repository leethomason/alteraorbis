#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"

using namespace Simple;

ChitBag::ChitBag()
{
	updateList.Resort( 0, false );	// no dupes - turn into a set.
}


ChitBag::~ChitBag()
{
	DeleteAll();
}


void ChitBag::DeleteAll()
{
	chits.RemoveAll();	// calles delete, since the list is set with "owned"
}


void ChitBag::AddChit( Chit* chit )
{
	chits.Push( chit );
	chit->OnAdd( this );
}


void ChitBag::RemoveChit( Chit* chit ) 
{
	chits.Detach( chit );
	chit->OnRemove();
}


void ChitBag::RequestUpdate( Chit* chit )
{
	updateList.Add( chit );
}


void ChitBag::DoTick( U32 delta )
{
	for( int i=0; i<chits.GetSize(); ++i ) {
		// FIXME break into 2 lists
		if ( chits[i]->NeedsTick() ) {
			chits[i]->DoTick( delta );
		}
	}
	for( int i=0; i<updateList.GetSize(); ++i ) {
		updateList[i]->DoUpdate();
	}
	updateList.RemoveAll();
}



Chit* ChitBag::CreateTestChit( Engine* engine, const char* assetName )
{
	Chit* chit = new Chit();
	chit->Add( new SpatialComponent() );
	chit->Add( new RenderComponent( engine, assetName ) );
	this->AddChit( chit );
	return chit;
}
