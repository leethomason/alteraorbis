#include "component.h"
#include "chit.h"


void Component::RequestUpdate()
{
	if ( parentChit ) {
		parentChit->RequestUpdate();
	}
}


void Component::SendMessage( const char* name, int id )
{
	if ( parentChit ) {
		parentChit->SendMessage( name, id );
	}
}


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


