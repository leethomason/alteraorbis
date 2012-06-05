#include "healthcomponent.h"
#include "gamelimits.h"
#include "../xegame/chit.h"
#include "../grinliz/glutil.h"


void HealthComponent::SetHealth( int h )
{
	if ( health != h ) {
		health = h;
		parentChit->SendMessage( HEALTH_MSG_CHANGED, this, 0 );
	}
}


void HealthComponent::SetMaxHealth( int h )
{
	if ( maxHealth != h ) {
		maxHealth = h;
		health = grinliz::Min( health, maxHealth );
		parentChit->SendMessage( HEALTH_MSG_CHANGED, this, 0 );
	}
}
