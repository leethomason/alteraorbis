#include "component.h"
#include "chit.h"

void Component::RequestUpdate()
{
	if ( parentChit ) {
		parentChit->RequestUpdate();
	}
}


void Component::SendMessage( int id )
{
	if ( parentChit ) {
		parentChit->SendMessage( Name(), id );
	}
}


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


