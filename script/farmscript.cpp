#include "farmscript.h"
#include "../xegame/chit.h"

static const int FARM_SCRIPT_CHECK = 2000;

FarmScript::FarmScript() : timer( 2000 )
{
}


void FarmScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	timer.Serialize( xs, "timer" );
	XarcClose( xs );
}


void FarmScript::Init()
{
	timer.SetPeriod( FARM_SCRIPT_CHECK + scriptContext->chit->random.Rand(FARM_SCRIPT_CHECK/16));
}


int FarmScript::DoTick( U32 delta )
{
	timer.Delta( delta );
	return timer.Next();
}
