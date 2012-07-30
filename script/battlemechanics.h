#ifndef BATTLE_MECHANICS_INCLUDED
#define BATTLE_MECHANICS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

class Chit;
class WeaponItem;
class Engine;

class BattleMechanics
{
public:
	static void MeleeAttack( Engine* engine, Chit* src, WeaponItem* weapon );

	// Returns true the melee attack can/does succeed. Note that any animation
	// is pure decoration, melee success is just based on relative positions.
	// This is simple, but could be improved.
	static bool InMeleeZone( Engine* engine,
							 Chit* src,
							 Chit* target );
};


#endif // BATTLE_MECHANICS_INCLUDED
