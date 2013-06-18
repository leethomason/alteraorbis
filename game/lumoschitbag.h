#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "census.h"
#include "../engine/map.h"

class WorldMap;
struct Wallet;
class CoreScript;

class LumosChitBag : public ChitBag,
					 public IMapGridUse
{
public:
	LumosChitBag();
	virtual ~LumosChitBag();

	virtual LumosChitBag* ToLumos() { return this; }

	void SetContext( Engine* e, WorldMap* wm ) { engine = e; worldMap = wm; }

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team );
	Chit* NewGoldChit( const grinliz::Vector3F& pos, int amount );
	Chit* NewCrystalChit( const grinliz::Vector3F& pos, int crystal, bool fuzzPos );
	Chit* NewWorkerChit( const grinliz::Vector3F& pos, int team );
	//Chit* NewBuilding( const grinliz::Vector2I& pos, int building, int team );

	// Creates enough chits to empty the wallet.
	void NewWalletChits( const grinliz::Vector3F& pos, const Wallet& wallet );
	void AddItem( const char* name, Chit* chit, Engine* engine, int team, int level );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	virtual int MapGridUse( int x, int y );
	CoreScript* IsBoundToCore( Chit* );
	// Get the core for this sector.
	CoreScript* GetCore( const grinliz::Vector2I& sector );

	Census census;

	static bool GoldFilter( Chit* chit );			// Is this gold?
	static bool GoldCrystalFilter( Chit* chit );	// Is this gold or crystal? (wallet items)
	static bool CoreFilter( Chit* chit );
	static bool WorkerFilter( Chit* chit );

private:

	static bool HasMapSpatialInUse( Chit* );

	grinliz::CDynArray<Chit*> inUseArr;
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	WorldMap* worldMap;
	grinliz::Random random;	// use the chit version, if possible, generally want to avoid high level random
};


#endif // LUMOS_CHIT_BAG_INCLUDED
