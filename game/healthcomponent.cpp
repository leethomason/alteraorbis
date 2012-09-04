#include "healthcomponent.h"
#include "gamelimits.h"

#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"

#include "../grinliz/glutil.h"

using namespace grinliz;


float HealthComponent::GetHealthFraction() const
{
	if ( !parentChit->GetItemComponent() )
		return 1.0f;

	GameItem* item = parentChit->GetItemComponent()->GetItem();
	float fraction = item->hp / item->TotalHP();
	return fraction;
}


void HealthComponent::DeltaHealth()
{
	if ( destroyed )
		return;

	GameItem* item = 0;
	if ( parentChit->GetItemComponent() ) {
		item = parentChit->GetItemComponent()->GetItem();
	}
	if ( item ) {
		if ( item->hp == 0 ) {
			parentChit->SendMessage( ChitMsg(MSG_CHIT_DESTROYED), this );
			GLLOG(( "Chit %3d destroyed.\n", parentChit->ID() ));
			GetChitBag()->QueueDelete( parentChit );
			destroyed = true;
		}
	}
}
