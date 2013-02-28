#ifndef COMPONENT_FACTORY_INCLUDED
#define COMPONENT_FACTORY_INCLUDED

class Component;
class Chit;
class Engine;
class WorldMap;
class LumosGame;
class Weather;
class Sim;
class Census;

class ComponentFactory {
public:
	ComponentFactory( Sim* p_sim, Census* p_census, Engine* p_engine, WorldMap* p_worldMap, Weather* p_weather, LumosGame* p_lumosGame ) 
		: sim( p_sim ),
		  census( p_census ),
		  engine( p_engine ),
		  worldMap( p_worldMap ),
		  weather( p_weather ),
		  lumosGame( p_lumosGame )
	{}

	Component* Factory( const char* name, Chit* chit ) const;

	Sim* GetSim() const				{ return sim; }
	Engine*	GetEngine() const		{ return engine; }
	WorldMap* GetWorldMap() const	{ return worldMap; }
	Weather* GetWeather() const		{ return weather; }
	LumosGame* GetGame() const		{ return lumosGame; }

private:
	Sim*		sim;
	Census*		census;
	Engine*		engine;
	WorldMap*	worldMap;
	Weather*	weather;
	LumosGame*	lumosGame;

};

#endif // COMPONENT_FACTORY_INCLUDED