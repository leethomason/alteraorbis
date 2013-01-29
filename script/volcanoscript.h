#ifndef VOLCANO_SCRIPT_INCLUDED
#define VOLCANO_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../grinliz/glvector.h"

class WorldMap;

class VolcanoScript : public IScript
{
public:
	VolcanoScript( WorldMap* map );
	virtual ~VolcanoScript()			{}

	virtual void Init( const ScriptContext& heap );
	virtual void Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element );
	virtual void Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer );
	virtual bool DoTick( const ScriptContext& ctx, U32 delta );

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	WorldMap*			worldMap;
};


#endif // VOLCANO_SCRIPT_INCLUDED