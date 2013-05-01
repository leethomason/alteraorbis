#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "census.h"
#include "../engine/map.h"

class WorldMap;

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
	void AddItem( const char* name, Chit* chit, Engine* engine, int team, int level );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );

	virtual int MapGridUse( int x, int y );

	Census census;
	// Is this gold?
	static bool GoldFilter( Chit* chit );
	// Is this something to pick up?
	static bool LootFilter( Chit* chit );

private:

	static bool HasMapSpatialInUse( Chit* );
	grinliz::CDynArray<Chit*> inUseArr;
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	WorldMap* worldMap;
};


#endif // LUMOS_CHIT_BAG_INCLUDED
