#ifndef CORESCRIPT_INCLUDED
#define CORESCRIPT_INCLUDED

#include "scriptcomponent.h"

class WorldMap;

class CoreScript : public IScript
{
public:
	CoreScript( WorldMap* map );
	virtual ~CoreScript();

	virtual void Init( const ScriptContext& heap );
	virtual void Serialize( const ScriptContext& ctx, XStream* xs );
	virtual void OnAdd( const ScriptContext& ctx );
	virtual void OnRemove( const ScriptContext& ctx )	{}

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "CoreScript"; }

private:
	WorldMap* worldMap;
};

#endif // CORESCRIPT_INCLUDED
