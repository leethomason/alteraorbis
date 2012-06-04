#include "healthcomponent.h"
#include "gamelimits.h"
#include "../grinliz/glutil.h"


void HealthComponent::SetHealth( int h )
{
	if ( health != h ) {
		health = h;
		SendMessage( HEALTH_MSG_CHANGED, true );
	}
}


void HealthComponent::SetMaxHealth( int h )
{
	if ( maxHealth != h ) {
		maxHealth = h;
		health = grinliz::Min( health, maxHealth );
		SendMessage( HEALTH_MSG_CHANGED, true );
	}
}