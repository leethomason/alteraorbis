#include "chitbag.h"
#include "chit.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"

using namespace Simple;
using namespace grinliz;

ChitBag::ChitBag()
{
	idPool = 0;
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


Chit* ChitBag::CreateChit()
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
	updateList.Add( chit );
}


void ChitBag::DoTick( U32 delta )
{
	for( int i=0; i<chits.GetSize(); ++i ) {
		Chit* c = chits[i].Value;
		if ( c->NeedsTick() ) {
			c->DoTick( delta );
		}
	}
	for( int i=0; i<updateList.GetSize(); ++i ) {
		updateList[i]->DoUpdate();
	}
	updateList.RemoveAll();
}

