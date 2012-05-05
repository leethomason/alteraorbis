#ifndef XENOENGINE_CHITBAG_INCLUDED
#define XENOENGINE_CHITBAG_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

#include "../engine/enginelimits.h"
#include "../SimpleLib/SimpleLib.h"

#include "chit.h"

class Engine;


class ChitBag
{
public:
	ChitBag();
	~ChitBag();

	void DeleteAll();
	Chit* NewChit();
	void  DeleteChit( Chit* );

	// Calls every chit that has a tick.
	void DoTick( U32 delta );		
	// Due to events, changes, etc. a chit may need an update, possibily in addition to, the tick.
	// Normally called automatically.
	void RequestUpdate( Chit* );

private:
	int idPool;
	Simple::CIndex< int, Chit*, 
					Simple::SValue, Simple::SOwnedPtr> chits;	// Owned chit pointers. Note that CIndex 
																// supports fast iteration, which is critical.
	Simple::CSortedVector<Chit*> updateList;					// <set> of chits that need the Update() called.
};


#endif // XENOENGINE_CHITBAG_INCLUDED