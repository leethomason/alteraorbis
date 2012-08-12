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
	void MeleeAttack( Engine* engine, Chit* src, IMeleeWeaponItem* weapon );

	// Returns true the melee attack can/does succeed. Note that any animation
	// is pure decoration, melee success is just based on relative positions.
	// This is simple, but could be improved.
	bool InMeleeZone( Engine* engine,
					  Chit* src,
					  Chit* target );

	static int PrimaryTeam( Chit* src );

private:
	grinliz::CDynArray<Chit*> hashQuery;
};


#endif // BATTLE_MECHANICS_INCLUDED
