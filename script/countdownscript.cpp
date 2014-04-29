#include "countdownscript.h"
#include "../game/healthcomponent.h"
#include "../game/lumoschitbag.h"
#include "../xegame/chit.h"

void CountDownScript::Serialize( XStream* xs )
{
	BeginSerialize(xs, Name());
	timer.Serialize( xs, "timer" );
	EndSerialize(xs);
}


int CountDownScript::DoTick( U32 delta )
{
	if ( timer.Delta( delta ) ) {
		parentChit->DeRez();
	}
	return timer.Next();
}


