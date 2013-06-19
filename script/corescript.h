#ifndef CORESCRIPT_INCLUDED
#define CORESCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../xegame/cticker.h"

class WorldMap;
class WorkQueue;
class LumosChitBag;

class CoreScript : public IScript
{
public:
	CoreScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine );
	virtual ~CoreScript();

	virtual void Init( const ScriptContext& heap );
	virtual void Serialize( const ScriptContext& ctx, XStream* xs );
	virtual void OnAdd( const ScriptContext& ctx );
	virtual void OnRemove( const ScriptContext& ctx );

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "CoreScript"; }
	virtual CoreScript* ToCoreScript() { return this; }

	bool AttachToCore( Chit* chit );
	Chit* GetAttached();
	WorkQueue* GetWorkQueue()	{ return workQueue; }

private:
	WorldMap*	worldMap;
	WorkQueue*	workQueue;
	CTicker		spawnTick;
	int			boundID;
};

#endif // CORESCRIPT_INCLUDED
