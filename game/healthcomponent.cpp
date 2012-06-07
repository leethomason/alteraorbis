#include "healthcomponent.h"
#include "gamelimits.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../grinliz/glutil.h"

using namespace grinliz;


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


void HealthComponent::DeltaHealth( int d )
{
	int oldHealth = health;
	SetHealth( Clamp( health+d, 0, maxHealth ));
	if ( oldHealth > 0 && health == 0 ) {
		parentChit->SendMessage( MSG_CHIT_DESTROYED, this, 0 );
		GetChitBag()->QueueDelete( parentChit );
	}
}

