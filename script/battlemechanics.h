#ifndef BATTLE_MECHANICS_INCLUDED
#define BATTLE_MECHANICS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

#include "../game/gameitem.h"

class Chit;
class Engine;

class BattleMechanics
{
public:
	static void MeleeAttack( Engine* engine, Chit* src, GameItem* weapon );

	// Returns true the melee attack can/does succeed. Note that any animation
	// is pure decoration, melee success is just based on relative positions.
	// This is simple, but could be improved.
	static bool InMeleeZone( Engine* engine,
							 Chit* src,
							 Chit* target );
};


#endif // BATTLE_MECHANICS_INCLUDED
