#include "itemcomponent.h"
#include "chit.h"
#include "../game/healthcomponent.h"

void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == MSG_CHIT_DAMAGE ) {
		const DamageDesc* dd = (const DamageDesc*) msg.Ptr();
		GLASSERT( dd );
		GLLOG(( "Chit %3d '%s' ", parentChit->ID(), item.Name() ));

		// FIXME: see if inventory can absorb first (shields)

		float hp = item.hp;
		item.AbsorbDamage( *dd, 0, "DAMAGE" );
		GLLOG(( "\n" ));

		if ( item.hp != hp ) {
			HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
			if ( hc ) {
				hc->DeltaHealth();
			}
		}
	}
}


bool ItemComponent::DoTick( U32 delta )
{
	return item.DoTick( delta );
}



