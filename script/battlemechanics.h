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
#include "../grinliz/glrandom.h"
#include "../grinliz/glrectangle.h"

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
	// 'target' can be null if targeting a point or block.
	static float MeleeRange( Chit* src, Chit* target );

	// Returns true the melee attack can/does succeed. Note that any animation
	// is pure decoration, melee success is just based on relative positions.
	// This is simple, but could be improved.
	static bool InMeleeZone(	Engine* engine,
								Chit* src,
								Chit* target );

	// Returns true if the melee attack - against a block - can/does succeed.
	static bool InMeleeZone(	Engine* engine,
								Chit* src,
								const grinliz::Vector2I& mapPos );

	static void CalcMeleeDamage( Chit* src, IMeleeWeaponItem* weapon, DamageDesc* );
	
	// Shooting ------------------- //
	static void Shoot(	ChitBag* bag, 
						Chit* src, 
						const grinliz::Vector3F& target,
						bool targetMoving,
						IRangedWeaponItem* weapon );


	// Radius at 1 unit distance. The shot is randomly placed within the sphere
	// at that distance. The shere part is tricky - doing volume calc for hit
	// chances.
	static float ComputeRadAt1(	GameItem* shooter, 
								IRangedWeaponItem* weapon,
								bool shooterMoving,
								bool targetMoving );
	// Returns the chance of hitting, between 0 and 1.
	static float ChanceToHit( float range, float radAt1, float targetDiameter=0.5f );
	// Returns the range at which the weapon has a 50% chance to hit.
	// 1m is assumed the default target diameter.
	static float EffectiveRange( float radAt1, float targetDiameter=0.5f, float chanceToHit=0.5f );
	
	// Computes an accurate leading shot. Returns a target. 
	// The trigger is optional.
	// Returns the leading target.
	static grinliz::Vector3F ComputeLeadingShot(	Chit* origin, 
													Chit* target, 
													grinliz::Vector3F* trigger );

	// Other --------------------- //

	static void GenerateExplosionMsgs( const DamageDesc& dd, const grinliz::Vector3F& origin, int originID, Engine* engine, grinliz::CDynArray<Chit*>* storage );

	static int PrimaryTeam( Chit* src );

private:
	static grinliz::Vector3F FuzzyAim( const grinliz::Vector3F& origin, const grinliz::Vector3F& target, float radiusAt1 );

	static grinliz::Vector3F ComputeLeadingShot(	const grinliz::Vector3F& origin,
													const grinliz::Vector3F& target,
													const grinliz::Vector3F& velocity,
													float speed );

	static grinliz::Random random;
};


#endif // BATTLE_MECHANICS_INCLUDED
