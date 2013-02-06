#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcontainer.h"
#include "../tinyxml2/tinyxml2.h"

#include "gamelimits.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class Texture;
class Chit;
class Weather;


class Sim
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapDAT, const char* mapXML, const char* gameXML );
	void Save( const char* mapDAT, const char* mapXML, const char* gameXML );

	void Draw( U32 delta );

	Texture*		GetMiniMapTexture();
	Chit*			GetPlayerChit();

	// use with caution: not a clear separation between sim and game
	Engine*			GetEngine()		{ return engine; }
	LumosChitBag*	GetChitBag()	{ return chitBag; }
	WorldMap*		GetWorldMap()	{ return worldMap; }

	// Set all rock to the nominal values
	void SetAllRock();
	void CreateVolcano( int x, int y, int size );
	// type=-1 will scan for natural plant choice
	void CreatePlant( int x, int y, int type );

	double DateInAge() const { return (double)timeInMinutes / (double)MINUTES_IN_AGE; }

private:
	void CreatePlayer( const grinliz::Vector2I& pos, const char* assetName );
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	Engine*			engine;
	LumosGame*		lumosGame;
	WorldMap*		worldMap;
	Weather*		weather;
	LumosChitBag*	chitBag;

	grinliz::Random	random;
	int playerID;
	int secondClock;
	int minuteClock;
	U32 timeInMinutes;
	int volcTimer;

	grinliz::CDynArray< Chit* >	queryArr;	// local; cached at object.
};


#endif // LUMOS_SIM_INCLUDED
