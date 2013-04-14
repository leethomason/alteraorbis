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

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team, const grinliz::Vector2F* wander );
	void AddItem( const char* name, Chit* chit, Engine* engine, int team=0, int level=0 );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );

	virtual int MapGridUse( int x, int y );

	Census census;

private:
	static bool HasMapSpatialInUse( Chit* );
	grinliz::CDynArray<Chit*> inUseArr;
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	WorldMap* worldMap;
};


#endif // LUMOS_CHIT_BAG_INCLUDED
