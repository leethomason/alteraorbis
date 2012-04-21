#include "component.h"
#include "chit.h"

void Component::RequestUpdate()
{
	if ( parentChit ) {
		parentChit->RequestUpdate();
	}
}

