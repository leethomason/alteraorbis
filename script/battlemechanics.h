/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BATTLE_MECHANICS_INCLUDED
#define BATTLE_MECHANICS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"

#include "../game/gameitem.h"

class Chit;
class Engine;
class ChitBag;
class DamageDesc;

class BattleMechanics
{
public:
	// Melee --------------------- //
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
	
	// Shooting ------------------- //
	void Shoot( ChitBag* bag, Chit* src, Chit* target, IRangedWeaponItem* weapon, const grinliz::Vector3F& pos );
	grinliz::Vector3F ComputeLeadingShot( const grinliz::Vector3F& origin,
										  const grinliz::Vector3F& target,
										  const grinliz::Vector3F& velocity,
										  float speed );

	// Other --------------------- //

	static void GenerateExplosionMsgs( const DamageDesc& dd, const grinliz::Vector3F& origin, Engine* engine, grinliz::CDynArray<Chit*>* storage );

	static int PrimaryTeam( Chit* src );

private:
	grinliz::CDynArray<Chit*> hashQuery;
};


#endif // BATTLE_MECHANICS_INCLUDED
