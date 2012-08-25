#include "component.h"
#include "chit.h"
#include "chitbag.h"

int Component::idPool = 1;

Chit* Component::GetChit( int id )
{
	if ( parentChit ) {
		return parentChit->GetChitBag()->GetChit( id );
	}
	return 0;
}


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


