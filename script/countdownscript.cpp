#include "countdownscript.h"
#include "../game/healthcomponent.h"
#include "../game/lumoschitbag.h"
#include "../xegame/chit.h"

void CountDownScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	timer.Serialize( xs, "timer" );
	XarcClose( xs );
}


int CountDownScript::DoTick( U32 delta )
{
	if ( timer.Delta( delta ) ) {
		HealthComponent* hc = scriptContext->chit->GetHealthComponent();
		GameItem* item      = scriptContext->chit->GetItem();

		// Try to delete the correct way (health to 0) 
		// else force a delete.
		if ( hc && item ) {
			item->hp = 0;
			scriptContext->chit->SetTickNeeded();
		}
		else {
			scriptContext->chitBag->QueueDelete( scriptContext->chit );
		}
	}
	return timer.Next();
}


