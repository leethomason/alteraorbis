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
		parentChit->SendMessage( ChitMsg(HEALTH_MSG_CHANGED), this );
	}
}


void HealthComponent::SetMaxHealth( int h )
{
	if ( maxHealth != h ) {
		maxHealth = h;
		health = grinliz::Min( health, maxHealth );
		parentChit->SendMessage( ChitMsg(HEALTH_MSG_CHANGED), this );
	}
}


void HealthComponent::DeltaHealth( int d )
{
	int oldHealth = health;
	SetHealth( Clamp( health+d, 0, maxHealth ));
	if ( oldHealth > 0 && health == 0 ) {
		parentChit->SendMessage( ChitMsg(MSG_CHIT_DESTROYED), this );
		GetChitBag()->QueueDelete( parentChit );
	}
}
