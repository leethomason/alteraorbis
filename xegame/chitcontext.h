#ifndef CHIT_CONTEXT_INCLUDED
#define CHIT_CONTEXT_INCLUDED

#include "../grinliz/glvector.h"
#include "../game/gamelimits.h"
#include "cticker.h"

class Engine;
class WorldMap;
class LumosGame;
class LumosChitBag;
class PhysicsSims;

class ChitContext
{
public:
	// cross-engine
	Engine*		engine = 0;

	// game specific
	WorldMap*	worldMap = 0;
	LumosGame*	game = 0;
	LumosChitBag* chitBag = 0;
	PhysicsSims* physicsSims = 0;
};


#endif // CHIT_CONTEXT_INCLUDED
