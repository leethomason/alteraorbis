#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "census.h"

class LumosChitBag : public ChitBag
{
public:
	LumosChitBag();
	virtual ~LumosChitBag() {}

	void SetEngine( Engine* e ) { engine = e; }

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );

	Census census;

private:
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
};


#endif // LUMOS_CHIT_BAG_INCLUDED
