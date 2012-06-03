#include "healthcomponent.h"
#include "gamelimits.h"


void HealthComponent::SetHealth( int h )
{
	if ( health != h ) {
		health = h;
		SendMessage( HEALTH_MSG_CHANGED, true );
	}
}
