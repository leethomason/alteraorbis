#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"

class LumosChitBag : public ChitBag
{
public:
	LumosChitBag() : engine(0)	{}
	virtual ~LumosChitBag() {}

	void SetEngine( Engine* e ) { engine = e; }

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );

private:
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	grinliz::Random random;

};


#endif // LUMOS_CHIT_BAG_INCLUDED
