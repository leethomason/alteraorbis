#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../engine/ufoutil.h"
#include "../SimpleLib/SimpleLib.h"
#include "chit.h"

class Engine;


class ChitBag
{
public:
	ChitBag();
	~ChitBag();

	void DeleteAll();
	void AddChit( Chit* );
	void RemoveChit( Chit* );

	// Calls every chit that has a tick.
	void DoTick( U32 delta );		
	// Due to events, changes, etc. a chit may need an update, possibily in addition to, the tick.
	// Normally called automatically.
	void RequestUpdate( Chit* );

	// Utility / Loading
	Chit* CreateTestChit( Engine* engine, const char* assetName );

private:
	Simple::CVector<Chit*, Simple::SOwnedPtr> chits;
	Simple::CSortedVector<Chit* > updateList;
};


#endif // XENOENGINE_CHITBAG_INCLUDED