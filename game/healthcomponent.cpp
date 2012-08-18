#include "healthcomponent.h"
#include "gamelimits.h"

#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"

#include "../grinliz/glutil.h"

using namespace grinliz;


/*
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
*/


float HealthComponent::GetHealthFraction() const
{
	float fraction = 1.0f;
	GameItem* item = 0;
	if ( parentChit->GetItemComponent() ) {
		item = parentChit->GetItemComponent()->GetItem();
	}
	if ( item ) {
		fraction = item->hp / item->TotalHP();
	}
	return fraction;
}



void HealthComponent::DeltaHealth( float d )
{
	GameItem* item = 0;
	if ( parentChit->GetItemComponent() ) {
		item = parentChit->GetItemComponent()->GetItem();
	}
	if ( item ) {
		float oldHealth = item->hp;
		item->hp = Clamp( oldHealth+d, 0.f, item->TotalHP() );
		GLLOG(( "Chit %3d '%10s' health %5.1f -> %5.1f\n",
			    parentChit->ID(), item->Name(), oldHealth, item->hp ));
		if ( oldHealth > 0 && item->hp == 0 ) {
			parentChit->SendMessage( ChitMsg(MSG_CHIT_DESTROYED), this );
			GLLOG(( "Chit %3d destroyed.\n", parentChit->ID() ));
			GetChitBag()->QueueDelete( parentChit );
		}
	}
}
