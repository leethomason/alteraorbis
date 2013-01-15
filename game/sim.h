#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class Texture;
class Chit;

class Sim
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapPNG, const char* mapDAT, const char* mapXML, const char* gameXML );
	void Save( const char* mapDAT, const char* mapXML, const char* gameXML );

	void Draw( U32 delta );

	Texture*		GetMiniMapTexture();
	Chit*			GetPlayerChit();

	// use with caution: not a clear separation between sim and game
	Engine*			GetEngine()		{ return engine; }
	LumosChitBag*	GetChitBag()	{ return chitBag; }

private:
	void CreatePlayer( const grinliz::Vector2I& pos, const char* assetName );

	Engine*			engine;
	LumosGame*		lumosGame;
	WorldMap*		worldMap;
	LumosChitBag*	chitBag;

	int playerID;
};


#endif // LUMOS_SIM_INCLUDED
