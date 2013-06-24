#ifndef LUMOS_SIM_INCLUDED
#define LUMOS_SIM_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcontainer.h"

#include "gamelimits.h"
#include "visitor.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class Texture;
class Chit;
class Weather;
class ReserveBank;

class Sim
{
public:
	Sim( LumosGame* game );
	~Sim();

	void DoTick( U32 deltaTime );
	void Draw3D( U32 deltaTime );
	void DrawDebugText();

	void Load( const char* mapDAT, const char* gameDAT );
	void Save( const char* mapDAT, const char* gameDAT );

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
	
	void CreatePlayer();
	void CreatePlayer( const grinliz::Vector2I& pos, const char* assetName );

private:
	void CreateCores();
	void CreateRockInOutland();

	Engine*			engine;
	LumosGame*		lumosGame;
	WorldMap*		worldMap;
	Weather*		weather;
	ReserveBank*	reserveBank;
	LumosChitBag*	chitBag;
	Visitors*		visitors;

	grinliz::Random	random;
	int playerID;
	int secondClock;
	int minuteClock;
	U32 timeInMinutes;
	int volcTimer;
	int currentVisitor;

	grinliz::CDynArray< Chit* >	queryArr;	// local; cached at object.
};


#endif // LUMOS_SIM_INCLUDED
