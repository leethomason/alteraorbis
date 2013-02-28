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
	ctx.census->cores.Push( ctx.chit->ID() );
}


void CoreScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XarcClose( xs );
}


void CoreScript::OnAdd( const ScriptContext& ctx )
{
	ctx.census->cores.Push( ctx.chit->ID() );
	// Cores are indestructable. They don't get removed.
}


int CoreScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	// spawn stuff.
	return 100;
}


