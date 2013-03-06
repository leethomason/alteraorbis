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

#include "../game/gameitem.h"
#include "../game/gamelimits.h"
#include "../game/healthcomponent.h"

#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/inventorycomponent.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glrandom.h"

#include "../engine/engine.h"
#include "../engine/camera.h"
#include "../engine/particle.h"

using namespace grinliz;

static const float SHOOTER_MOVE_MULT = 0.5f;
static const float TARGET_MOVE_MULT  = 0.8f;
static const float BASE_ACCURACY     = 0.03f;


/*static*/ int BattleMechanics::PrimaryTeam( Chit* src )
{
	int primaryTeam = -1;
	if ( src->GetItemComponent() ) {
		primaryTeam = src->GetItemComponent()->GetItem()->primaryTeam;
	}
	return primaryTeam;
}


/*static*/ float BattleMechanics::MeleeRange( Chit* src, Chit* target )
{
	RenderComponent* srcRender = src->GetRenderComponent();
	RenderComponent* targetRender = target->GetRenderComponent();

	if ( srcRender && targetRender ) {
		float meleeRange = srcRender->RadiusOfBase()*1.5f + targetRender->RadiusOfBase();
		return meleeRange;
	}
	return 0;
}


bool BattleMechanics::InMeleeZone(	Engine* engine,
									Chit* src,
									Chit* target )
{
	ComponentSet srcComp(   src,     Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	ComponentSet targetComp( target, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );

	if ( !srcComp.okay || !targetComp.okay )
		return false;

	// Check range up front and early out.
	const float meleeRange = MeleeRange( src, target );
	const float range = ( targetComp.spatial->GetPosition2D() - srcComp.spatial->GetPosition2D() ).Length();
	if ( range > meleeRange )
		return false;

	int test = IntersectRayCircle( targetComp.spatial->GetPosition2D(),
								   targetComp.render->RadiusOfBase(),
								   srcComp.spatial->GetPosition2D(),
								   srcComp.spatial->GetHeading2D() );

	bool intersect = ( test == INTERSECT || test == INSIDE );
	return intersect;
}


void BattleMechanics::MeleeAttack( Engine* engine, Chit* src, IMeleeWeaponItem* weapon )
{
	GLASSERT( engine && src && weapon );
	ChitBag* chitBag = src->GetChitBag();
	GLASSERT( chitBag );

	U32 absTime = chitBag->AbsTime();
	if( !weapon->Ready()) {
		return;
	}
	weapon->Use();
	
	int primaryTeam = PrimaryTeam( src );

	// Get origin and direction of melee attack,
	// then send messages to everyone hit. Everything
	// with a spatial component is tracked by the 
	// chitBag, so it's a very handy query.

	Vector2F srcPos = src->GetSpatialComponent()->GetPosition2D();
	Rectangle2F b;
	b.min = srcPos; b.max = srcPos;
	b.Outset( MELEE_RANGE + MAX_BASE_RADIUS );
	chitBag->QuerySpatialHash( &hashQuery, b, src, 0 );

	for( int i=0; i<hashQuery.Size(); ++i ) {
		Chit* target = hashQuery[i];

		// Melee damage is chaos. Don't hit your own team.
		if ( PrimaryTeam( target ) == primaryTeam ) {
			continue;
		}

		if ( InMeleeZone( engine, src, target )) {
			// FIXME: account for armor, shields, etc. etc.
			// FIXME: account for knockback (physics move), catching fire, etc.
			HealthComponent* targetHealth = GET_COMPONENT( target, HealthComponent );
			ComponentSet targetComp( target, Chit::ITEM_BIT | ComponentSet::IS_ALIVE );

			if ( targetHealth && targetComp.okay ) {
				target->SetTickNeeded();	// Fire up tick to handle health effects over time
				DamageDesc dd;
				CalcMeleeDamage( src, weapon, &dd );

				GLLOG(( "Chit %3d '%s' using '%s' hit %3d '%s'\n", 
						src->ID(), src->GetItemComponent()->GetItem()->Name(),
						weapon->GetItem()->Name(),
						target->ID(), targetComp.item->Name() ));

				target->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &dd ), 0 );
			}
		}
	}
}



void BattleMechanics::CalcMeleeDamage( Chit* src, IMeleeWeaponItem* weapon, DamageDesc* dd )
{
	GameItem* item = weapon->GetItem();

	InventoryComponent* inv = src->GetInventoryComponent();
	GLASSERT( inv && item );
	if ( !inv ) return;

	// A chain is: sword[0] -> claw[1] -> creature[2]
	grinliz::CArray< GameItem*, 4 > chain;
	inv->GetChain( item, &chain );
	GLASSERT( !chain.Empty() );
	GLASSERT( chain.Size() == 2 || chain.Size() == 3 );

	// Now that we have a chain, how do effects commute?
	// Generally left:
	//		A flaming creature will impart FIRE to a regular sword.
	// But not right:
	//		A flaming sword won't light up the creature.
	// 
	// Or at least that is the theory so far.

	int effect = 0;
	for( int i=chain.Size()-1; i>=0; --i ) {
		effect |= ( chain[i]->flags & GameItem::EFFECT_MASK );
	}
	// Remove explosive melee effect. It would only be funny 10 or 20 times.
	effect ^= GameItem::EFFECT_EXPLOSIVE;

	// Now about mass.
	// Use the sum; using another approach can yield surprising damage calc.
	float mass = 0;
	for( int i=0; i<chain.Size(); ++i ) {
		mass += chain[i]->mass;
	}
	static const float STRIKE_RATIO_INV = 1.0f / 5.0f;

	float trait0 = chain[0]->stats.Damage();
	float trait2 = chain[chain.Size()-1]->stats.Damage(); 

	dd->damage = mass * chain[0]->meleeDamage * STRIKE_RATIO_INV * trait0 * trait2;
	dd->effects = effect;
}


float BattleMechanics::ComputeRadAt1(	GameItem* shooter, 
										IRangedWeaponItem* weapon,
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
		accuracy *= shooter->stats.Accuracy();
	}
	if ( weapon ) {
		accuracy *= weapon->GetItem()->stats.Accuracy();
	}

 	float radAt1 = BASE_ACCURACY / accuracy;
	return radAt1;
}


void BattleMechanics::Shoot( ChitBag* bag, Chit* src, Chit* target, IRangedWeaponItem* weapon )
{
	GLASSERT( weapon->Ready() );
	bool okay = weapon->Use();
	if ( !okay ) {
		return;
	}
	if ( !target->GetRenderComponent() ) {
		return;
	}
	GameItem* item = weapon->GetItem();

	Vector3F p0, p1;
	Vector3F aimAt = ComputeLeadingShot( src, target, &p0, &p1 );
	
	float radAt1 = ComputeRadAt1( src->GetItem(), weapon, src->GetMoveComponent()->IsMoving(), target->GetMoveComponent()->IsMoving() );
	Vector3F dir = FuzzyAim( p0, p1, radAt1 );

	Bolt* bolt = bag->NewBolt();
	bolt->head = p0 + dir;
	bolt->len = 1.0f;
	bolt->dir = dir;
	if ( item->flags & GameItem::EFFECT_FIRE )
		bolt->color.Set( 1, 0, 0, 1 );	// FIXME: real color based on item
	else
		bolt->color.Set( 0, 1, 0, 1 );	// FIXME: real color based on item
	bolt->chitID = src->ID();
	bolt->damage = item->rangedDamage * item->stats.Damage();
	bolt->effect = item->Effects();
	bolt->particle  = (item->flags & GameItem::RENDER_TRAIL) ? true : false;
	bolt->speed = item->CalcBoltSpeed();
}


float BattleMechanics::EffectiveRange( float radAt1, float targetDiameter )
{
	float r0 = MIN_EFFECTIVE_RANGE;
	float r1 = MAX_EFFECTIVE_RANGE;
	float c0 = ChanceToHit( r0, radAt1, targetDiameter );
	float c1 = ChanceToHit( r1, radAt1, targetDiameter );
	static const float C = 0.5f;

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
		// Cliffs make me nervous using a N-R search.
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


Vector3F BattleMechanics::ComputeLeadingShot( Chit* origin, Chit* target, Vector3F* p0, Vector3F* p1 )
{
	Vector3F trigger = { 0, 0, 0 };
	Vector3F t = { 0, 0, 0 };
	Vector3F v = { 0, 0, 0 };
	if ( target->GetRenderComponent() ) {
		target->GetRenderComponent()->CalcTarget( &t );
	}
	else if ( target->GetSpatialComponent() ) {
		t = target->GetSpatialComponent()->GetPosition();
		t.y += 0.5f;
	}
	else {
		GLASSERT( 0 );
	}

	if ( target->GetMoveComponent() ) {
		target->GetMoveComponent()->CalcVelocity( &v );
	}

	GameItem* item = origin->GetItem();
	GLASSERT( item );
	float speed = item->CalcBoltSpeed();

	if ( origin->GetRenderComponent() ) {
		RenderComponent* rc = origin->GetRenderComponent();
		if ( rc && rc->GetMetaData( "trigger", &trigger )) {
			// have value;
		}
		else if ( rc ) {
			rc->CalcTarget( &trigger );
		}
		else if ( origin->GetSpatialComponent() ) {
			trigger = origin->GetSpatialComponent()->GetPosition();
			trigger.y += 0.5f;
		}
		else {
			GLASSERT( 0 );
		}
	}
	if ( p0 ) {
		*p0 = trigger;
	}
	if ( p1 ) {
		*p1 = t;
	}

	return ComputeLeadingShot( trigger, t, v, speed );
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


void BattleMechanics::GenerateExplosionMsgs( const DamageDesc& _dd, const Vector3F& origin, int originID, Engine* engine, CDynArray<Chit*>* hashQuery )
{
	Rectangle2F rect;
	rect.Set( origin.x, origin.z, origin.x, origin.z );
	rect.Outset( EXPLOSIVE_RANGE );
	WorldScript::QueryChits( rect, engine, hashQuery );

	GLLOG(( "<Explosion> (%.1f,%.1f,%.1f)\n", origin.x, origin.y, origin.z ));
	for( int i=0; i<hashQuery->Size(); ++i ) {
		Vector3F target = { 0, 0, 0 };
		Chit* chit = (*hashQuery)[i];

		RenderComponent* rc = chit->GetRenderComponent();
		if ( rc ) {
			rc->CalcTarget( &target );

	#if 0
			// This is correct, and keeps explosions from going through walls.
			// But is unsitifying, too, since models stope explosions.
			Vector3F hit;
			Model* m = engine->IntersectModel( origin, target-origin, RANGE, TEST_TRI, 0, 0, 0, &hit );
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
			Vector3F hit = target;
			{
				{
	#endif
					// HIT!
					float len = (hit-origin).Length();
					// Scale the damage based on the range.
					if ( len < EXPLOSIVE_RANGE ) {
						DamageDesc dd = _dd;
						float t = (EXPLOSIVE_RANGE-len)/EXPLOSIVE_RANGE;
						dd.damage *= t;

						ChitMsg msg( ChitMsg::CHIT_DAMAGE, 1, &dd );
						msg.vector = target - origin;
						msg.vector.Normalize();
						msg.vector.Multiply( Lerp( 2.f, 4.f, t ));
						msg.originID = originID;
						chit->SendMessage( msg, 0 );
					}
				}
			}
		}
	}
	GLLOG(( "</Explosion>\n" ));
}

