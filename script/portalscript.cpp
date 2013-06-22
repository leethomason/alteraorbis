#include "portalscript.h"

using namespace grinliz;

PortalScript::PortalScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine ) 
	: worldMap( map )
{
}


PortalScript::~PortalScript()
{
}

void PortalScript::Init( const ScriptContext& ctx )
{
}


void PortalScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XarcClose( xs );
}


void PortalScript::OnAdd( const ScriptContext& ctx )
{
}


void PortalScript::OnRemove( const ScriptContext& ctx )
{
}


int PortalScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	return 0;
}