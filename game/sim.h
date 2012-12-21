#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"


class Engine;
class WorldMap;
class LumosGame;

class Sim
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapPNG, const char* mapXML );

private:
	LumosGame*	lumosGame;
	Engine*		engine;
	WorldMap*	worldMap;
};


#endif // LUMOS_SIM_INCLUDED
