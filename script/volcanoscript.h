#ifndef VOLCANO_SCRIPT_INCLUDED
#define VOLCANO_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../grinliz/glvector.h"

class WorldMap;

class VolcanoScript : public IScript
{
public:
	VolcanoScript( WorldMap* map, int maxSize );
	virtual ~VolcanoScript()			{}

	virtual void Init( const ScriptContext& heap );
	virtual void Serialize( const ScriptContext& ctx, XStream* xs );

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "VolcanoScript"; }

private:
	WorldMap*	worldMap;
	int			size;
	int			maxSize;
};


#endif // VOLCANO_SCRIPT_INCLUDED