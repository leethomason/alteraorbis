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

#include "battlemechanics.h"
#include "worldscript.h"
#include "procedural.h"

#include "../game/gameitem.h"
#include "../game/gamelimits.h"
#include "../game/healthcomponent.h"
#include "../game/worldmap.h"
#include "../game/lumoschitbag.h"
#include "../game/team.h"
#include "../game/mapspatialcomponent.h"

#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/chitcontext.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glrandom.h"

#include "../engine/engine.h"
#include "../engine/camera.h"
#include "../engine/particle.h"

#include "../audio/xenoaudio.h"

using namespace grinliz;

static const float SHOOTER_MOVE_MULT = 0.5f;
static const float TARGET_MOVE_MULT  = 0.8f;
static const float BASE_ACCURACY     = 0.03f;
static const float MELEE_DOT_PRODUCT = 0.7f;


/*static*/ Random BattleMechanics::random;


bool BattleMechanics::InMeleeZone(	Engine* engine,
									Chit* src,
									Chit* target )
{
	// Buildings are a special challenge.
	MapSpatialComponent* msc = GET_SUB_COMPONENT(target, SpatialComponent, MapSpatialComponent);
	if (msc) {
		Rectangle2I bounds = msc->Bounds();
		Rectangle2F aabb;
		aabb.Set(float(bounds.min.x), float(bounds.min.y),
			     float(bounds.max.x + 1), float(bounds.max.y + 1));
		Vector2F nearest = { 0, 0 };
		const float range = PointAABBDistance(ToWorld2F(src->Position()), aabb, &nearest);

		return range < MELEE_RANGE;
	}

	// Check range up front and early out.
	const float range = ( target->Position() - src->Position() ).Length();
	if ( range > MELEE_RANGE )
		return false;

	RenderComponent* targetRC = target->GetRenderComponent();
	if (!targetRC) return false;

	int test = IntersectRayCircle( ToWorld2F(target->Position()),
								   targetRC->RadiusOfBase(),
								   ToWorld2F(src->Position()),
								   src->Heading2D() );

	bool intersect = ( test == INTERSECT || test == INSIDE );
	return intersect;
}


bool BattleMechanics::InMeleeZone(	Engine* engine,
									Chit* src,
									const Vector2I& mapPos )
{
	// Check range up front and early out.
	Rectangle2F aabb;
	aabb.Set( (float)mapPos.x, (float)mapPos.y, (float)(mapPos.x+1), (float)(mapPos.y+1) );
	Vector2F nearest = { 0, 0 };
	const float range = PointAABBDistance( ToWorld2F(src->Position()), aabb, &nearest );

	return range < MELEE_RANGE;
}


bool BattleMechanics::MeleeAttack( Engine* engine, Chit* src, MeleeWeapon* weapon )
{
	GLASSERT( engine && src && weapon );
	ChitBag* chitBag = src->Context()->chitBag;
	GLASSERT( chitBag );

//	U32 absTime = chitBag->AbsTime();
	
	// Get origin and direction of melee attack,
	// then send messages to everyone hit. Everything
	// with a spatial component is tracked by the 
	// chitBag, so it's a very handy query.

	Vector2F srcPos = ToWorld2F(src->Position());
	Vector2F srcHeading = src->Heading2D();

	Rectangle2F b;
	b.min = srcPos; b.max = srcPos;
	b.Outset( MELEE_RANGE + MAX_BASE_RADIUS + 0.8f );

	CChitArray hashQuery;
	ChitAcceptAll accept;
	chitBag->QuerySpatialHash( &hashQuery, b, src, &accept );

	DamageDesc dd;
	CalcMeleeDamage( src->GetItem(), weapon, &dd );
	float damage = dd.damage;
	ChitDamageInfo info( dd );
	info.originID = src->ID();
	info.awardXP  = true;
	info.isMelee  = true;
	info.isExplosion = false;
	info.originOfImpact = src->Position();
	BattleFilter filter;
	WeaponFilter weaponFilter;
	bool impact = false;

	// Check for chit impacts.
	for( int i=0; i<hashQuery.Size(); ++i ) {
		Chit* target = hashQuery[i];

		// Melee damage is chaos. Don't hit your own friends.
		// Arguably, shouldn't hit neutrals either. But that
		// keeps a rampage from going through ruins.
		if ( filter.Accept( target ) ) {
			if ( Team::Instance()->GetRelationship( src, target ) == ERelate::FRIEND ) {
				continue;
			}
		}

		// Also, don't melee hit weapons. I can't imagine this
		// is the intent and the weapon toll is massive. Ranged
		// and explosive still take out weapons.
		if (weaponFilter.Accept(target)) {
			continue;
		}

		if ( InMeleeZone( engine, src, target )) {
			HealthComponent* targetHealth = target->GetHealthComponent();

			if ( targetHealth ) {
				target->SetTickNeeded();	// Fire up tick to handle health effects over time

				Vector2F toTarget = ToWorld2F(target->Position()) - srcPos;
				toTarget.Normalize();
				float dot = DotProduct(toTarget, srcHeading);
				float scale = 0.5f + 0.5f*dot;

//				GLLOG(( "Chit %3d '%s' using '%s' hit %3d '%s'\n", 
//						src->ID(), src->GetItemComponent()->GetItem()->Name(),
//						weapon->GetItem()->Name(),
//						target->ID(), targetComp.item->Name() ));

				dd.damage = damage*scale;	// the info holds a reference - non obvious
				target->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &info ), 0 );
				impact = true;
			}
		}
	}

	// Check for block impacts.
	Rectangle2I bi;
	bi.Set( (int)b.min.x, (int)b.min.y, (int)ceilf(b.max.x), (int)ceilf(b.max.y) );
	WorldMap* wm = engine->GetMap()->ToWorldMap();
	GLASSERT( wm );

	for( int y=bi.min.y; y<=bi.max.y; ++y ) {
		for( int x=bi.min.x; x<=bi.max.x; ++x ) {
			Vector2I mapPos = { x, y };
			if ( InMeleeZone( engine, src, mapPos )) {
				const WorldGrid& wg = wm->GetWorldGrid( x, y );
				if ( wg.IsObject() ) {
					Vector2F toTarget = ToWorld2F(mapPos) - srcPos;
					toTarget.Normalize();
					float dot = DotProduct(toTarget, srcHeading);
					float scale = 0.5f + 0.5f*dot;
					dd.damage = damage * scale;

					Vector3I voxel = { x, 0, y };
					wm->VoxelHit( voxel, dd );
					impact = true;
				}
			}
		}
	}

	if (impact && XenoAudio::Instance()) {
		IString impactSound = weapon->keyValues.GetIString("impactSound");
		if (!impactSound.empty()) {
			Vector3F pos3 = src->Position();
			XenoAudio::Instance()->PlayVariation(impactSound, src->random.Rand(), &pos3 );
		}
	}
	return impact;
}



void BattleMechanics::CalcMeleeDamage( const GameItem* weilder, const MeleeWeapon* weapon, DamageDesc* dd )
{
	GLASSERT( weapon );
	if ( !weapon ) return;

	int effect = weapon->flags & GameItem::EFFECT_MASK;

	// Remove explosive melee effect. It would only be funny 10 or 20 times.
	effect &= (~GameItem::EFFECT_EXPLOSIVE);

	// I keep going back and forth on the weilder, and the imapact it
	// has on the damage. There is no "hit" calculation with melee;
	// they always hit. So the only variable is the damage. Heavier units
	// should probably do more damage, but there isn't much real
	// variation there. But do add in the the affect of the weilders 
	// Damage() skill.
	if (weilder)
		dd->damage = weapon->Damage() * weilder->Traits().Damage();
	else
		dd->damage = weapon->Damage();
	dd->effects = effect;
}


float BattleMechanics::ComputeRadAt1(	const GameItem* shooter, 
										const RangedWeapon* weapon,
										bool shooterMoving,
										bool targetMoving )
{
	float accuracy = 1;

	if ( shooterMoving ) {
		accuracy *= SHOOTER_MOVE_MULT;
	}
	if ( targetMoving ) {
		accuracy *= TARGET_MOVE_MULT;
	}

	if ( shooter  ) {
		accuracy *= shooter->Traits().Accuracy();
	}
	if ( weapon ) {
		accuracy *= weapon->Accuracy();
	}

 	float radAt1 = BASE_ACCURACY / accuracy;
	return radAt1;
}


void BattleMechanics::Shoot(ChitBag* bag, Chit* src, const grinliz::Vector3F& _target, bool targetMoving, RangedWeapon* weapon)
{
	Vector3F aimAt = _target;
	GLASSERT(weapon);
	GLASSERT(weapon->CanShoot());
	bool okay = weapon->Shoot(src);
	if (!okay) {
		GLASSERT(okay);
		return;
	}
	Vector3F p0;
	GLASSERT(src->GetRenderComponent());
	src->GetRenderComponent()->CalcTrigger(&p0, 0);
	GLASSERT(p0.y > 0);
	GLASSERT((p0 - src->Position()).Length() < 3.0);	// sanity check something hasn't gone way wrong

	IString sound = weapon->keyValues.GetIString(ISC::sound);
	if (!sound.empty() && XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(sound, weapon->ID(), &p0);
	}

	// Explosives shoot at feet.
	if (weapon->flags & GameItem::EFFECT_EXPLOSIVE) {
		aimAt.y = 0;
	}

	float radAt1 = ComputeRadAt1(src->GetItem(), weapon, src->GetMoveComponent()->IsMoving(), targetMoving);
	Vector3F dir = FuzzyAim(p0, aimAt, radAt1);

	bag->ToLumos()->NewBolt(p0, dir, weapon->Effects(), src->ID(),
							weapon->Damage(),
							weapon->BoltSpeed(),
							(weapon->flags & GameItem::RENDER_TRAIL) ? true : false);
}


float BattleMechanics::EffectiveRange( float radAt1, float targetDiameter, float chance )
{
	float r0 = MIN_EFFECTIVE_RANGE;
	float r1 = MAX_EFFECTIVE_RANGE;
	float c0 = ChanceToHit( r0, radAt1, targetDiameter );
	float c1 = ChanceToHit( r1, radAt1, targetDiameter );
	const float C = chance;

	if ( c0 <= C ) {
		return MIN_EFFECTIVE_RANGE;
	}
	if ( c1 >= C ) {
		return MAX_EFFECTIVE_RANGE;
	}

	// c = c0 + m*r;
	// r = (c - c0) / m;
	float c = 0;
	float r = 0;

	for( int i=0; i<5; ++i ) {
		// Cliffs make me nervous using a N-R search. Use
		// a binary search instead so it doesn't behave badly.
		r = Mean(r0,r1);
		c = ChanceToHit( r, radAt1, targetDiameter );

		if ( c < C ) {
			r1 = r;
			c1 = c;
		}
		else {
			r0 = r;
			c0 = c;
		}
	}
	return r;
}


float BattleMechanics::ChanceToHit( float range, float radAt1, float targetDiameter )
{
	/*
		There is real math to be done here. The computation below assumes
		that any point on the sphere is equally likely.
	*/

	// Volume of a sphere:
	// V = (4/3)PI r^3
	//
	// Partial sphere volume:
	//	http://mathworld.wolfram.com/SphericalCap.html
	//
	// Vcap = (PI/3) * h^2 * (3*R - h)
	//

	float rSphere = radAt1 * range;
	float rCyl    = targetDiameter * 0.5f;

	if ( rSphere <= rCyl ) {
		return 1.0f;
	}
	if ( rCyl == 0.0f ) {
		return 0.0f;
	}

	float lenCyl = 2.0f * sqrtf( rSphere*rSphere - rCyl*rCyl );
	float hCap = rSphere - 0.5f*lenCyl;

	float vCap = (PI/3.0f) * hCap*hCap * (3.0f*rCyl - hCap); 
	float vInside = PI*rCyl*rCyl*lenCyl + 2.0f*vCap;
	float vSphere = (4.0f/3.0f)*PI*rSphere*rSphere*rSphere;
	float result = vInside / vSphere;
	return result;
}



Vector3F BattleMechanics::FuzzyAim( const Vector3F& pos, const Vector3F& aimAt, float radiusAt1 )
{
	Vector3F dir = aimAt - pos;
	float len = dir.Length();

	if ( radiusAt1 > 0 && len > 0 ) {

		/* Compute a point in the sphere. All points should be equally likely,
		   remembering the target is the 2 projection, this makes the center
		   of the sphere more likely than the outside.

		   I'm not convined this algorithm achieves the desired randomness, however.

		   Choosing a random direction and a random length, however, biases the sphere
		   to the center. This is necessarily bad, but it's a different result.
		*/
		float t = FLT_MAX;
		Vector3F rv = { 0, 0, 0 };
		while ( t > 1.0f ) {
			rv.Set( -1.0f+random.Uniform()*2.0f, -1.0f+random.Uniform()*2.0f, -1.0f+random.Uniform()*2.0f );
			t = rv.LengthSquared();
		}

		Vector3F aimAtPrim = aimAt + rv * radiusAt1 * len;
		dir = aimAtPrim - pos;
	}
	dir.Normalize();
	return dir;
}


Vector3F BattleMechanics::ComputeLeadingShot( Chit* origin, Chit* target, float boltSpeed, Vector3F* p0 )
{
	Vector3F trigger = { 0, 0, 0 };
	Vector3F t = { 0, 0, 0 };
	Vector3F v = { 0, 0, 0 };
	if ( target->GetRenderComponent() ) {
		target->GetRenderComponent()->CalcTarget( &t );
	}
	else {
		t = target->Position();
		t.y += 0.5f;
	}

	if ( target->GetMoveComponent() ) {
		v = target->GetMoveComponent()->Velocity();
	}

	GameItem* item = origin->GetItem();
	GLASSERT( item );
	(void)item;

	if ( origin->GetRenderComponent() ) {
		RenderComponent* rc = origin->GetRenderComponent();
		if ( rc && rc->GetMetaData( HARDPOINT_TRIGGER, &trigger )) {
			// have value;
		}
		else if ( rc ) {
			rc->CalcTarget( &trigger );
		}
		else {
			trigger = origin->Position();
			trigger.y += 0.5f;
		}
	}
	if ( p0 ) {
		*p0 = trigger;
	}
	return ComputeLeadingShot( trigger, t, v, boltSpeed );
}


float BattleMechanics::MeleeDPTU( const GameItem* wielder, const MeleeWeapon* weapon)
{
	int meleeTime = weapon->CalcMeleeCycleTime();
	DamageDesc dd;
	CalcMeleeDamage( wielder, weapon, &dd );
	return dd.damage*1000.0f/(float)meleeTime;
}


float BattleMechanics::RangedDPTU( const RangedWeapon* weapon, bool continuous )
{
	float secpershot = 1000.0f / (float)weapon->CooldownTime();
	float csecpershot = (float)weapon->ClipCap() * 1000.0f / 
							(float)(weapon->CooldownTime()*weapon->ClipCap() + weapon->ReloadTime()); 

	float dps  = weapon->Damage() * 0.5f * secpershot;
	float cdps = weapon->Damage() * 0.5f * csecpershot;

	return continuous ? cdps : dps;
}


Vector3F BattleMechanics::ComputeLeadingShot(	const grinliz::Vector3F& origin,
												const grinliz::Vector3F& target,
												const grinliz::Vector3F& v,
												float speed )
{

	// FIRE! compute the "best" shot. Simple, but not trivial, math. Stack overflow, you are mighty:
	// http://stackoverflow.com/questions/2248876/2d-game-fire-at-a-moving-target-by-predicting-intersection-of-projectile-and-un
	// a := sqr(target.velocityX) + sqr(target.velocityY) - sqr(projectile_speed)
	// b := 2 * (target.velocityX * (target.startX - cannon.X) + target.velocityY * (target.startY - cannon.Y))
	// c := sqr(target.startX - cannon.X) + sqr(target.startY - cannon.Y)
	//
		
	float a = SquareF( v.x ) + SquareF(  v.y ) + SquareF( v.z ) - SquareF( speed );
	float b = 2.0f * ( v.x*(target.x - origin.x) + v.y*(target.y - origin.y) + v.z*(target.z - origin.z));
	float c = SquareF( target.x - origin.x ) + SquareF( target.y - origin.y) + SquareF( target.z - origin.z);

	float disc = b*b - 4.0f*a*c;
	Vector3F aimAt = target;	// default to shooting straight at target if there is an error

	if ( disc > 0.01f ) {
		float t1 = ( -b + sqrtf( disc ) ) / (2.0f * a );
		float t2 = ( -b - sqrtf( disc ) ) / (2.0f * a );

		float t = 0;
		if ( t1 > 0 && t2 > 0 ) t = Min( t1, t2 );
		else if ( t1 > 0 ) t = t1;
		else t = t2;

		aimAt = target + t * v;
	}
	return aimAt;
}


void BattleMechanics::GenerateExplosionMsgs( const DamageDesc& dd, const Vector3F& origin, int originID, Engine* engine, ChitBag* chitBag )
{
	Vector2F origin2 = { origin.x, origin.z };

	CChitArray hashQuery;
	ChitAcceptAll acceptAll;
	chitBag->QuerySpatialHash( &hashQuery, origin2, EXPLOSIVE_RANGE, 0, &acceptAll );

//	GLLOG(( "<Explosion> (%.1f,%.1f,%.1f)\n", origin.x, origin.y, origin.z ));
	//WorldMap* worldMap = engine->GetMap() ? engine->GetMap()->ToWorldMap() : 0;

	for( int i=0; i<hashQuery.Size(); ++i ) {
		Vector3F target = { 0, 0, 0 };
		Chit* chit = hashQuery[i];

		// Check that the explosion doesn't go through walls, in a general quasi accurate way
		// This doesn't work either - stops short of an unpathable target.
		//Vector2F chit2 = chit->GetPosition2D();
		//if ( worldMap && !worldMap->HasStraightPath( origin2, chit2 )) {
		//	continue;
		//}

		RenderComponent* rc = chit->GetRenderComponent();
		if ( rc ) {
			rc->CalcTarget( &target );

	#if 0
			// This is correct, and keeps explosions from going through walls.
			// But is unsitifying, too, since models stope explosions.
			Vector3F hit;
			Model* m = engine->IntersectModelVoxel( origin, target-origin, RANGE, TEST_TRI, 0, 0, 0, &hit );
	#ifdef DEBUG_EXPLOSION
			if ( m ) 
				DebugLine( origin, hit, 1, 0, 0 );
			else
				DebugLine( origin, target );
	#endif
			if ( m ) {
				// Did we hit the current chit? Use the ignoreList 'in reverse': if
				// we hit any component of the Chit, we hit the chit.
				CArray<const Model*, EL_MAX_METADATA+2> targetList;
				rc->GetModelList( &targetList );
				if ( targetList.Find( m ) >= 0 ) {
	#else
			//Vector3F hit = target;
			{
				{
	#endif
					// HIT!
					float len = (target-origin).Length();
					if ( len < EXPLOSIVE_RANGE ) {

						ChitDamageInfo info( dd );
						info.originID = originID;
						info.awardXP  = true;
						info.isMelee  = false;
						info.isExplosion = true;
						info.originOfImpact = origin;

						ChitMsg msg( ChitMsg::CHIT_DAMAGE, 1, &info );
						chit->SendMessage( msg, 0 );
					}
				}
			}
		}
	}
//	GLLOG(( "</Explosion>\n" ));
}

void BattleMechanics::GenerateExplosion(const DamageDesc& dd,
										const grinliz::Vector3F& pos,
										int originID,
										Engine* engine,
										ChitBag* chitBag,
										WorldMap* worldMap)
{
	// Audio:
	int seed = int(pos.x + pos.y + pos.z) + originID;
	if (XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(ISC::explosionWAV, Random::Hash8(seed), &pos);
	}

	// Particles:
	ParticleSystem* ps = engine->particleSystem;
	ParticleDef def = ps->GetPD( ISC::explosion );

	Color4F color = WeaponGen::GetEffectColor(dd.effects);
	def.color = { color.x, color.y, color.z, 1.0f };
	ps->EmitPD( def, pos, V3F_UP, 0 );

	// Physics:
	if (chitBag) {
		GenerateExplosionMsgs(dd, pos, originID, engine, chitBag);
	}

	// Physics, voxel:
	if (pos.z < EXPLOSIVE_RANGE) {
		Rectangle2I r2i;
		r2i.Set(LRint(pos.x - EXPLOSIVE_RANGE), LRint(pos.y - EXPLOSIVE_RANGE), LRint(pos.x + EXPLOSIVE_RANGE), LRint(pos.y + EXPLOSIVE_RANGE));
		for (Rectangle2IIterator it(r2i); !it.Done(); it.Next()) {
			Vector3I voxel = { it.Pos().x, 0, it.Pos().y };
			worldMap->VoxelHit(voxel, dd);
		}
	}
}