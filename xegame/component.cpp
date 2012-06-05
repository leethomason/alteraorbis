#include "component.h"
#include "chit.h"

int Component::idPool = 1;

void Component::RequestUpdate()
{
	if ( parentChit ) {
		parentChit->RequestUpdate();
	}
}


/*
void Component::SendMessage( int id, bool sendToSiblings )
{
	if ( parentChit ) {
		parentChit->SendMessage( id, sendToSiblings ? this : 0 );
	}
}
*/


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


