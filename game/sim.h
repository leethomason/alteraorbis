#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

class Engine;
class WorldMap;
class LumosGame;
class ChitBag;
class ItemStorage;

class Sim
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapPNG, const char* mapXML );
	void Draw( U32 delta );

private:
	void CreatePlayer( const grinliz::Vector2I& pos, const char* assetName );

	Engine*			engine;
	LumosGame*		lumosGame;
	WorldMap*		worldMap;
	ItemStorage*	itemStorage;
	ChitBag*		chitBag;
};


#endif // LUMOS_SIM_INCLUDED
