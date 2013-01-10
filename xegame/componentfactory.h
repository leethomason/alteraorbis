#ifndef COMPONENT_FACTORY_INCLUDED
#define COMPONENT_FACTORY_INCLUDED

class Component;
class Chit;
class Engine;
class WorldMap;
class LumosGame;

class ComponentFactory {
public:
	ComponentFactory( Engine* p_engine, WorldMap* p_worldMap, LumosGame* p_lumosGame ) 
		: engine( p_engine ),
		  worldMap( p_worldMap ),
		  lumosGame( p_lumosGame )
	{}

	Component* Factory( const char* name, Chit* chit ) const;

private:
	Engine*		engine;
	WorldMap*	worldMap;
	LumosGame*	lumosGame;

};

#endif // COMPONENT_FACTORY_INCLUDED