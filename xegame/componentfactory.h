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
class LumosChitBag;

class ComponentFactory {
public:
	ComponentFactory( Sim* p_sim, Census* p_census, Engine* p_engine, WorldMap* p_worldMap, LumosChitBag* p_chitBag, Weather* p_weather, LumosGame* p_lumosGame ) 
		: sim( p_sim ),
		  census( p_census ),
		  engine( p_engine ),
		  worldMap( p_worldMap ),
		  chitBag( p_chitBag ),
		  weather( p_weather ),
		  lumosGame( p_lumosGame )
	{}

	Component* Factory( const char* name, Chit* chit ) const;

	Sim* GetSim() const				{ return sim; }
	Engine*	GetEngine() const		{ return engine; }
	WorldMap* GetWorldMap() const	{ return worldMap; }
	Weather* GetWeather() const		{ return weather; }
	LumosGame* GetGame() const		{ return lumosGame; }
	LumosChitBag* GetChitBag() const	{ return chitBag; }

private:
	Sim*		sim;
	Census*		census;
	Engine*		engine;
	WorldMap*	worldMap;
	LumosChitBag* chitBag;
	Weather*	weather;
	LumosGame*	lumosGame;

};

#endif // COMPONENT_FACTORY_INCLUDED