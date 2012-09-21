#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"

class LumosChitBag : public ChitBag
{
public:
	LumosChitBag()			{}
	virtual ~LumosChitBag() {}

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at );
};


#endif // LUMOS_CHIT_BAG_INCLUDED
