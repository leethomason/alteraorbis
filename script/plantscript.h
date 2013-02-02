#ifndef PLANT_SCRIPT_INCLUDED
#define PLANT_SCRIPT_INCLUDED

#include "scriptcomponent.h"

class WorldMap;
class Engine;

class PlantScript : public IScript
{
public:
	PlantScript( Engine* engine, WorldMap* map, int type );
	virtual ~PlantScript()	{}

	virtual void Init( const ScriptContext& heap );
	virtual void Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element );
	virtual void Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer );
	virtual bool DoTick( const ScriptContext& ctx, U32 delta );

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );
	void SetRenderComponent( Chit* chit );

	Engine*		engine;
	WorldMap*	worldMap;
	int			type;		// 0-7, fern, tree, etc.
	int			stage;		// 0-3
	U32			age;
	U32			ageAtStage;
};

#endif // PLANT_SCRIPT_INCLUDED