#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/ufoutil.h"

class Chit;


class ChitBag
{
public:
	ChitBag();
	~ChitBag();

	void AddChit( Chit* );
	void RemoveChit( Chit* );

	void DoTick( U32 delta );

private:
	CDynArray<Chit*> chits;
};


#endif // XENOENGINE_CHITBAG_INCLUDED