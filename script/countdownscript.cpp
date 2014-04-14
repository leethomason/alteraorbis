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
		scriptContext->chit->DeRez();
	}
	return timer.Next();
}


