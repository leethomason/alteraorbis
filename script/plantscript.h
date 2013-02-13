#ifndef PLANT_SCRIPT_INCLUDED
#define PLANT_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../grinliz/glrandom.h"

class WorldMap;
class Engine;
class Weather;
class GameItem;
class Chit;
class Sim;

class PlantScript : public IScript
{
public:
	PlantScript( Sim* sim, Engine* engine, WorldMap* map, Weather* weather, int type );
	virtual ~PlantScript()	{}

	virtual void Init( const ScriptContext& heap );
	virtual void Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element );
	virtual void Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer );
	virtual void Serialize( const ScriptContext& ctx, DBItem item );

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "PlantScript"; }

	static GameItem* IsPlant( Chit* chit, int* type, int* stage );

	int Type() const	{ return type; }
	int Stage() const	{ return stage; }

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );
	void SetRenderComponent( Chit* chit );
	const GameItem* GetResource();

	Sim*		sim;
	Engine*		engine;
	WorldMap*	worldMap;
	Weather*	weather;
	grinliz::Vector2I	lightTap;		// where to check for a shadow. (only check one spot.)
	float				lightTapYMult;	// y multiple to get shadow height
	int			type;		// 0-7, fern, tree, etc.
	int			stage;		// 0-3
	U32			age;
	U32			ageAtStage;
	int			growTimer;
	int			sporeTimer;
};

#endif // PLANT_SCRIPT_INCLUDED