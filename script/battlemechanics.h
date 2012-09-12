#ifndef BATTLE_MECHANICS_INCLUDED
#define BATTLE_MECHANICS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

#include "../game/gameitem.h"

class Chit;
class Engine;
class ChitBag;

class BattleMechanics
{
public:
	void MeleeAttack( Engine* engine, Chit* src, IMeleeWeaponItem* weapon );

	// Returns the melee range of 2 chits, or 0 if none.
	static float MeleeRange( Chit* src, Chit* target );

	// Returns true the melee attack can/does succeed. Note that any animation
	// is pure decoration, melee success is just based on relative positions.
	// This is simple, but could be improved.
	bool InMeleeZone( Engine* engine,
					  Chit* src,
					  Chit* target );

	void CalcMeleeDamage( Chit* src, IMeleeWeaponItem* weapon, DamageDesc* );
	void Shoot( ChitBag* bag, Chit* src, Chit* target, IRangedWeaponItem* weapon, const grinliz::Vector3F& pos );

	static int PrimaryTeam( Chit* src );

private:
	grinliz::CDynArray<Chit*> hashQuery;
};


#endif // BATTLE_MECHANICS_INCLUDED
