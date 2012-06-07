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
	static bool InMeleeZone(	const grinliz::Vector2F& origin, 
								const grinliz::Vector2F& dirNormal, 
								const grinliz::Vector2F& target );
};


#endif // BATTLE_MECHANICS_INCLUDED
