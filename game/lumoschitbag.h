#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "census.h"

class WorldMap;

class LumosChitBag : public ChitBag
{
public:
	LumosChitBag();
	virtual ~LumosChitBag() {}

	void SetContext( Engine* e, WorldMap* wm ) { engine = e; worldMap = wm; }

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team, const grinliz::Vector2F* wander );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );

	Census census;

private:
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	WorldMap* worldMap;
};


#endif // LUMOS_CHIT_BAG_INCLUDED
