#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/ufoutil.h"

class Chit;
class Engine;


class ChitBag
{
public:
	ChitBag();
	~ChitBag();
	void DeleteAll();

	void AddChit( Chit* );
	void RemoveChit( Chit* );

	void DoTick( U32 delta );

	Chit* CreateTestChit( Engine* engine, const char* assetName );

private:
	CDynArray<Chit*> chits;
};


#endif // XENOENGINE_CHITBAG_INCLUDED