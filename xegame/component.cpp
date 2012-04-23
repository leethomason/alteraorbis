#include "component.h"
#include "chit.h"

void Component::RequestUpdate()
{
	if ( parentChit ) {
		parentChit->RequestUpdate();
	}
}


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


