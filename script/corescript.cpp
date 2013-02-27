#include "corescript.h"
#include "../game/census.h"
#include "../game/lumoschitbag.h"
#include "../xegame/chit.h"

CoreScript::CoreScript( WorldMap* map ) : worldMap( map )
{

}


CoreScript::~CoreScript()
{
}


void CoreScript::Init( const ScriptContext& ctx )
{
	Census* census = &(static_cast<LumosChitBag*>( ctx.chit->GetChitBag())->census);
	census->cores.Push( ctx.chit->ID() );
}


void CoreScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	if ( xs->Loading() ) {
		Census* census = &(static_cast<LumosChitBag*>( ctx.chit->GetChitBag())->census);
		census->cores.Push( ctx.chit->ID() );
	}
	XarcClose( xs );
}


int CoreScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	// spawn stuff.
}


