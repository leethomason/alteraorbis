#ifndef PORTAL_SCRIPT_INCLUDED
#define PORTAL_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../xegame/cticker.h"

class WorldMap;
class WorkQueue;
class LumosChitBag;

class PortalScript : public IScript
{
public:
	PortalScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine );
	virtual ~PortalScript();

	virtual void Init( const ScriptContext& heap );
	virtual void Serialize( const ScriptContext& ctx, XStream* xs );
	virtual void OnAdd( const ScriptContext& ctx );
	virtual void OnRemove( const ScriptContext& ctx );

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "PortalScript"; }
	virtual PortalScript* ToPortalScript() { return this; }

private:
	WorldMap*	worldMap;
	WorkQueue*	workQueue;
};


#endif // PORTAL_SCRIPT_INCLUDED
