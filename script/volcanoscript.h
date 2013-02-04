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
	virtual void Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element );
	virtual void Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer );
	virtual bool DoTick( const ScriptContext& ctx, U32 delta );
	virtual const char* ScriptName() { return "VolcanoScript"; }

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	WorldMap*	worldMap;
	int			size;
	int			maxSize;
};


#endif // VOLCANO_SCRIPT_INCLUDED