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

#include "aicomponent.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"
#include "gameitem.h"
#include "lumoschitbag.h"
#include "mapspatialcomponent.h"

#include "../script/battlemechanics.h"
#include "../script/plantscript.h"
#include "../script/worldgen.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

#include "../grinliz/glrectangle.h"
#include <climits>

using namespace grinliz;

static const float NORMAL_AWARENESS		= 10.0f;
static const float LOSE_AWARENESS		= 16.0f;
static const float SHOOT_ANGLE			= 10.0f;	// variation from heading that we can shoot
static const float SHOOT_ANGLE_DOT		=  0.985f;	// same number, as dot product.
static const float WANDER_RADIUS		=  5.0f;

#define AI_OUTPUT

AIComponent::AIComponent( Engine* _engine, WorldMap* _map ) : rethink( 1200 )
{
	engine = _engine;
	map = _map;
	currentAction = 0;
	currentTarget = 0;
	focusOnTarget = false;
	focusedMove = false;
	randomWander = false;
	aiMode = NORMAL_MODE;
	awareness.Zero();
	//wanderOrigin.Zero();
	//wanderRadius = 0;
	wanderTime = 0;
	//wanderFlags = 0;
	debugFlag = false;
}


AIComponent::~AIComponent()
{
}


void AIComponent::Serialize( XStream* xs )
{
	// FIXME check
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, aiMode );
	XARC_SER( xs, currentAction );
	XARC_SER( xs, currentTarget );
	XARC_SER( xs, focusOnTarget );
	XARC_SER( xs, focusedMove );
	//XARC_SER( xs, wanderRadius );
	XARC_SER( xs, wanderTime );
//	XARC_SER( xs, wanderFlags );

	//XARC_SER( xs, wanderOrigin );
	rethink.Serialize( xs, "rethink" );
	this->EndSerialize( xs );
}


void AIComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	if ( parentChit->GetLumosChitBag() ) {
		parentChit->GetLumosChitBag()->census.ais += 1;
	}
}


void AIComponent::OnRemove()
{
	if ( parentChit->GetLumosChitBag() ) {
		parentChit->GetLumosChitBag()->census.ais -= 1;
	}
	super::OnRemove();
}


int AIComponent::GetTeamStatus( Chit* other )
{
	// FIXME: placeholder friend/enemy logic
	GameItem* thisItem  = parentChit->GetItem();
	GameItem* otherItem = other->GetItem();

	int thisTeam = thisItem ? thisItem->primaryTeam : 0;
	int otherTeam = otherItem ? otherItem->primaryTeam : 0;

	if ( thisTeam == 0 || otherTeam == 0 ) {
		return NEUTRAL;
	}
	if ( thisTeam != otherTeam ) {
		return ENEMY;
	}
	return FRIENDLY;
}


bool AIComponent::LineOfSight( const ComponentSet& thisComp, Chit* t )
{
	ComponentSet target( t, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );

	Vector3F origin, dest;
	IRangedWeaponItem* weapon = thisComp.itemComponent->GetRangedWeapon( &origin );
	GLASSERT( weapon );

	target.render->GetMetaData( IStringConst::ktarget, &dest );

	Vector3F dir = dest - origin;
	float length = dir.Length() + 1.0f;	// a little extra just in case
	CArray<const Model*, EL_MAX_METADATA+2> ignore, targetModels;
	thisComp.render->GetModelList( &ignore );

	Vector3F at;

	Model* m = engine->IntersectModel( origin, dir, length, TEST_TRI, 0, 0, ignore.Mem(), &at );
	if ( m ) {
		target.render->GetModelList( &targetModels );
		return targetModels.Find( m ) >= 0;
	}
	return false;
}


static bool FEFilter( Chit* c ) {
	// Tricky stuff. This will return F/E units:
	if( GET_COMPONENT( c, AIComponent ))
		return true;
	// This probably isn't quite right. But
	// automated things?
	if ( c->GetMoveComponent() && c->GetItem() ) {
		return true;
	}
	return false;
}

void AIComponent::GetFriendEnemyLists()
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return;
	Vector2F center = sc->GetPosition2D();

	Rectangle2F zone = awareness;
	if ( zone.Area() == 0 ) {
		zone.min = zone.max = center;
		zone.Outset( NORMAL_AWARENESS );
	}
	if ( map->UsingSectors() ) {
		Rectangle2I ri = SectorData::InnerSectorBounds( center.x, center.y );
		Rectangle2F rf;
		rf.Set( (float)ri.min.x, (float)ri.min.y, (float)ri.max.x, (float)ri.max.y );

		zone.DoIntersection( rf );
	}

	friendList.Clear();
	enemyList.Clear();

	const CDynArray<Chit*>& chitArr = GetChitBag()->QuerySpatialHash( zone, parentChit, FEFilter );
	for( int i=0; i<chitArr.Size(); ++i ) {
		int status = GetTeamStatus( chitArr[i] );
		if ( status == ENEMY ) {
			if ( enemyList.HasCap() ) {
				enemyList.Push( chitArr[i]->ID());
			}
		}
		else if ( status == FRIENDLY ) {
			if ( friendList.HasCap() ) {
				friendList.Push( chitArr[i]->ID());
			}
		}
	}

	// Add the currentTarget back in, if we lost it. But only if
	// it hasn't gone too far away.
	Chit* currentTargetChit = GetChitBag()->GetChit( currentTarget );
	if ( currentTargetChit && currentTargetChit->GetSpatialComponent() ) {
		Vector2F targetCenter = currentTargetChit->GetSpatialComponent()->GetPosition2D();
		if (    (targetCenter - center).LengthSquared() < LOSE_AWARENESS*LOSE_AWARENESS			// close enough OR
			 || focusOnTarget )																// focused
		{
			int i = enemyList.Find( currentTarget );
			if ( i < 0 && enemyList.HasCap() ) {
				enemyList.Push( currentTarget );
			}
		}
	}
}


#if 0
// Won't write output if there isn't a result.
float AIComponent::CalcFlockMove( const ComponentSet& thisComp, grinliz::Vector2F* dir )
{
	if ( friendList.Empty() ) {
		return 0;
	}

	// Only the friends in front of us. Keeps Chits
	// from wandering off into space.
	Vector2F d = { 0, 0 };
	int count = 0;

	static const float TOO_CLOSE		= 1.0f;
	static const float JUST_RIGHT		= 2.0f;
	static const float IGNORE_ENEMY		= NORMAL_AWARENESS*0.5f;	// enemies don't effect flocking if other mechanisms will dominate.
	static const float MIN_ANGLE_DOT	= 0.26f;	// 75 degree

	Vector2F pos = thisComp.spatial->GetPosition2D();
	Vector2F heading = thisComp.spatial->GetHeading2D();

	for( int i=0; i<friendList.Size(); ++i ) {
		Chit* c = GetChitBag()->GetChit( friendList[i] );
		if ( c && c->GetSpatialComponent() ) {
			SpatialComponent*  sc = c->GetSpatialComponent();

			Vector2F delta = sc->GetPosition2D() - pos;
			float len = delta.Length();
			delta.Normalize();
			//float dot = DotProduct( delta, heading );

			float effect = 0;
			if ( len < JUST_RIGHT ) {
				effect = -1.f + len/JUST_RIGHT;
			}
			else {
				effect = JUST_RIGHT / len;
			}
			d = d + effect * delta;
		}
	}

	float len = d.Length();
//	if ( len > 0.01f ) {
//		d.Normalize();
//		*dir = d;
//		return len / (float)(count);
//	}
//	return 0.0f;
	return Clamp( len, 0.0f, 1.0f );
}
#endif


Chit* AIComponent::Closest( const ComponentSet& thisComp, CArray<int, MAX_TRACK>* list )
{
	float best = FLT_MAX;
	Chit* chit = 0;

	for( int i=0; i<list->Size(); ++i ) {
		Chit* enemyChit = GetChit( (*list)[i] );
		ComponentSet enemy( enemyChit, Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
		if ( enemy.okay ) 
		{
			float len2 = (enemy.spatial->GetPosition() - thisComp.spatial->GetPosition()).LengthSquared();
			if ( len2 < best ) {
				best = len2;
				chit = enemy.chit;
			}
		}
	}
	return chit;
}


void AIComponent::DoMove( const ComponentSet& thisComp )
{
	if ( aiMode == BATTLE_MODE ) {
		// Run & Gun
		float utilityRunAndGun = 0.0f;
		Chit* targetRunAndGun = 0;

		IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
		GameItem* ranged = rangedWeapon ? rangedWeapon->GetItem() : 0;

		if ( ranged ) {
			Vector3F heading = thisComp.spatial->GetHeading();

			if ( ranged->CanUse() ) {
				float radAt1 = BattleMechanics::ComputeRadAt1( thisComp.chit->GetItem(), 
															   ranged->ToRangedWeapon(),
															   true,
															   true );	// Doesn't matter to utility.

				for( int k=0; k<enemyList.Size(); ++k ) {
					ComponentSet enemy( GetChitBag()->GetChit(enemyList[k]), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
					if ( !enemy.okay ) {
						continue;
					}
					Vector3F p0;
					Vector3F p1 = BattleMechanics::ComputeLeadingShot( thisComp.chit, enemy.chit, &p0 );
					Vector3F normal = p1 - p0;
					normal.Normalize();
					float range = (p0-p1).Length();

					if ( range < EXPLOSIVE_RANGE*3.0f ) {
						// Don't run & gun explosives if too close.
						continue;
					}

					if ( DotProduct( normal, heading ) > SHOOT_ANGLE_DOT ) {
						// Wow - can take the shot!
						float u = BattleMechanics::ChanceToHit( range, radAt1 );

						if ( u > utilityRunAndGun ) {
							utilityRunAndGun = u;
							targetRunAndGun = enemy.chit;
						}
					}
				}
			}
			float utilityReload = 0.0f;
			GameItem* rangedItem = ranged->GetItem();
			if ( rangedItem->CanReload() ) {
				utilityReload = 1.0f - rangedItem->RoundsFraction();
			}
			if ( utilityReload > 0 || utilityRunAndGun > 0 ) {
				#ifdef AI_OUTPUT
					if ( debugFlag ) {
						GLOUTPUT(( "ID=%d Move: RunAndGun=%.2f Reload=%.2f ", thisComp.chit->ID(), utilityRunAndGun, utilityReload ));
					}
				#endif
				if ( utilityRunAndGun > utilityReload ) {
					GLASSERT( targetRunAndGun );
					battleMechanics.Shoot(	GetChitBag(), 
											thisComp.chit, 
											targetRunAndGun, 
											ranged->ToRangedWeapon() );
					#ifdef AI_OUTPUT
					if ( debugFlag ) {
						GLOUTPUT(( "->RunAndGun\n" ));
					}
					#endif
				}
				else {
					ranged->Reload();
					#ifdef AI_OUTPUT
					if ( debugFlag ) {
						GLOUTPUT(( "->Reload\n" ));
					}
					#endif
				}
			}
		}
	}
}


void AIComponent::DoShoot( const ComponentSet& thisComp )
{
	bool pointed = false;
	ComponentSet target( GetChitBag()->GetChit( currentTarget ), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
	if ( !target.okay ) {
		currentAction = NO_ACTION;
		return;
	}

	Vector3F leading = battleMechanics.ComputeLeadingShot( thisComp.chit, target.chit, 0 );
	Vector2F leading2D = { leading.x, leading.z };

	// Rotate to target.
	Vector2F heading = thisComp.spatial->GetHeading2D();
	float headingAngle = RotationXZDegrees( heading.x, heading.y );

	Vector2F normalToTarget = leading2D - thisComp.spatial->GetPosition2D();
	float distanceToTarget = normalToTarget.Length();
	normalToTarget.Normalize();
	float angleToTarget = RotationXZDegrees( normalToTarget.x, normalToTarget.y );

	if ( fabsf( headingAngle - angleToTarget ) < SHOOT_ANGLE ) {
		// all good.
	}
	else {
		// Rotate to target.
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		if ( pmc ) {
			pmc->QueueDest( thisComp.spatial->GetPosition2D(), angleToTarget );
		}
		return;
	}

	IRangedWeaponItem* weapon = thisComp.itemComponent->GetRangedWeapon( 0 );
	if ( weapon ) {
		GameItem* item = weapon->GetItem();
		if ( item->HasRound() ) {
			// Has round. May be in cooldown.
			if ( item->CanUse() ) {
				battleMechanics.Shoot(	GetChitBag(), 
										parentChit, 
										target.chit, 
										weapon );
			}
		}
		else {
			// Out of ammo - do something else.
			currentAction = NO_ACTION;
		}
	}
}


void AIComponent::DoMelee( const ComponentSet& thisComp )
{
	IMeleeWeaponItem* weapon = thisComp.itemComponent->GetMeleeWeapon();
	ComponentSet target( GetChitBag()->GetChit( currentTarget ), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );

	if ( !weapon || !target.okay ) {
		currentAction = NO_ACTION;
		return;
	}
	GameItem* item = weapon->GetItem();

	// Are we close enough to hit? Then swing. Else move to target.
	if ( battleMechanics.InMeleeZone( engine, parentChit, target.chit ) ) {
		GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
	}
	else {
		// Move to target.
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		if ( pmc ) {
			Vector2F targetPos = target.spatial->GetPosition2D();
			Vector2F pos = thisComp.spatial->GetPosition2D();
			Vector2F dest = { -1, -1 };
			pmc->QueuedDest( &dest );

			float delta = ( dest-targetPos ).Length();
			float distanceToTarget = (targetPos - pos).Length();

			// If the error is greater than distance to, then re-path.
			if ( delta > distanceToTarget * 0.25f ) {
				pmc->QueueDest( target.chit );
			}
		}
		else {
			currentAction = NO_ACTION;
			return;
		}
	}
}


void AIComponent::OnChitEvent( const ChitEvent& event )
{
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay ) {
		return;
	}

	if ( event.ID() == ChitEvent::AWARENESS ) {
		awareness = event.AreaOfEffect();
		parentChit->SetTickNeeded();
	}
	else {
		super::OnChitEvent( event );
	}
}


void AIComponent::Think( const ComponentSet& thisComp )
{
	if ( aiMode == NORMAL_MODE ) {
		ThinkWander( thisComp );
	}
	else if ( aiMode == BATTLE_MODE ) {
		ThinkBattle( thisComp );
	}
};


void AIComponent::FocusedMove( const grinliz::Vector2F& dest, const Vector2I* sector )
{
	PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
	if ( pmc ) {
		pmc->QueueDest( dest, -1, sector );
		currentAction = NO_ACTION;
		focusedMove = true;
	}
}


void AIComponent::FocusedTarget( Chit* chit )
{
	aiMode = BATTLE_MODE;
	currentTarget = chit->ID();
	focusOnTarget = true;
	focusedMove = false;
}


Vector2F AIComponent::WanderOrigin( const ComponentSet& thisComp ) const
{
	Vector2F pos = thisComp.spatial->GetPosition2D();
	Vector2I m = { (int)pos.x/SECTOR_SIZE, (int)pos.y/SECTOR_SIZE };
	const SectorData& sd = map->GetWorldInfo().GetSector( m );
	Vector2F center = { (float)(sd.x+SECTOR_SIZE/2), (float)(sd.y+SECTOR_SIZE/2) };
	if ( sd.HasCore() )	{
		center.Set( (float)sd.core.x + 0.5f, (float)sd.core.y + 0.5f );
	}
	return center;
}


Vector2F AIComponent::ThinkWanderCircle( const ComponentSet& thisComp )
{
	// In a circle?
	// This turns out to be creepy. Worth keeping for something that is,
	// in fact, creepy.
	Vector2F dest = WanderOrigin( thisComp );
	Random random( thisComp.chit->ID() );

	float angleUniform = random.Uniform();
	float lenUniform   = 0.25f + 0.75f*random.Uniform();

	static const U32 PERIOD = 40*1000;
	U32 t = wanderTime % PERIOD;
	float timeUniform = (float)t / (float)PERIOD;

	angleUniform += timeUniform;
	float angle = angleUniform * 2.0f * PI;
	Vector2F v = { cosf(angle), sinf(angle) };
		
	v = v * (lenUniform * WANDER_RADIUS);

	dest = WanderOrigin( thisComp ) + v;
	return dest;
}


Vector2F AIComponent::ThinkWanderRandom( const ComponentSet& thisComp )
{
	Vector2F dest = WanderOrigin( thisComp );
	dest.x += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
	dest.y += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
	return dest;
}


Vector2F AIComponent::ThinkWanderFlock( const ComponentSet& thisComp )
{
	Vector2F origin = thisComp.spatial->GetPosition2D();

	static const int NPLANTS = 4;
	static const float PLANT_AWARE = 3.0f;
	static const float TOO_CLOSE = 2.0f;

	// +1 for origin, +4 for plants
	CArray<Vector2F, MAX_TRACK+1+NPLANTS> pos;
	for( int i=0; i<friendList.Size(); ++i ) {
		Chit* c = parentChit->GetChitBag()->GetChit( friendList[i] );
		if ( c && c->GetSpatialComponent() ) {
			Vector2F v = c->GetSpatialComponent()->GetPosition2D();
			pos.Push( v );
		}
	}
	pos.Push( WanderOrigin( thisComp ) );	// the origin is a friend.

	// And plants are friends.
	Rectangle2F r;
	r.min = r.max = origin;
	r.Outset( PLANT_AWARE );

	const CDynArray<Chit*>& plants = parentChit->GetChitBag()->QuerySpatialHash( r, 0, PlantScript::PassablePlantFilter );
	for( int i=0; i<plants.Size() && i<NPLANTS; ++i ) {
		pos.Push( plants[i]->GetSpatialComponent()->GetPosition2D() );
	}

	Vector2F mean = origin;
	// Get close to friends.
	for( int i=0; i<pos.Size(); ++i ) {
		mean = mean + pos[i];
	}
	Vector2F dest = mean * (1.0f/(float)(1+pos.Size()));
	Vector2F heading = thisComp.spatial->GetHeading2D();

	// But not too close.
	for( int pass=0; pass<2; ++pass ) {
		for( int i=0; i<pos.Size(); ++i ) {
			if ( (pos[i] - dest).LengthSquared() < TOO_CLOSE*TOO_CLOSE ) {
				dest += heading * TOO_CLOSE;
			}
		}
	}
	return dest;
}


void AIComponent::ThinkWander( const ComponentSet& thisComp )
{
	// Wander in some sort of directed fashion.
	// - get close to friends
	// - but not too close
	// - given a choice, get close to plants.
	// - occasionally randomly wander about

	Vector2F dest = { 0, 0 };

	int flags = parentChit->GetItem() ? (parentChit->GetItem()->flags & GameItem::AI_WANDER_MASK) : 0;
	if ( flags == 0 ) {
		rethink.Set( 2000 );
		currentAction = WANDER;
		return;
	}
	if ( randomWander ) {
		dest = ThinkWanderRandom( thisComp );
	}
	else if ( flags == GameItem::AI_WANDER_HERD ) {
		dest = ThinkWanderFlock( thisComp );
	}
	else if ( flags == GameItem::AI_WANDER_CIRCLE ) {
		dest = ThinkWanderCircle( thisComp );
	}

	PathMoveComponent* pmc = GET_COMPONENT( thisComp.chit, PathMoveComponent );
	if ( pmc ) {
		pmc->QueueDest( dest );
	}
	rethink.Set( 10*1000 + thisComp.chit->random.Rand( 10*1000 ));
	currentAction = WANDER;
}


void AIComponent::ThinkBattle( const ComponentSet& thisComp )
{
	PathMoveComponent* pmc = GET_COMPONENT( thisComp.chit, PathMoveComponent );
	if ( !pmc ) {
		currentAction = NO_ACTION;
		return;
	}
	const Vector3F& pos = thisComp.spatial->GetPosition();
	Vector2F pos2 = { pos.x, pos.z };
	
	// The current ranged weapon.
	IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
	// The current melee weapon.
	IMeleeWeaponItem* meleeWeapon = thisComp.itemComponent->GetMeleeWeapon();

	enum {
		OPTION_FLOCK_MOVE,		// Move to better position with allies (not too close, not too far)
		OPTION_MOVE_TO_RANGE,	// Move to shooting range or striking range
		OPTION_MELEE,			// Focused melee attack
		OPTION_SHOOT,			// Stand and shoot. (Don't pay run-n-gun accuracy penalty.)
		NUM_OPTIONS
	};

	float utility[NUM_OPTIONS] = { 0,0,0,0 };
	Chit* target[NUM_OPTIONS]  = { 0,0,0,0 };

	// Moves are always to the target location, since intermediate location could
	// cause very strange pathing. Time is set to when to "rethink". If we are moving
	// to melee for example, the time is set to when we should actually switch to
	// melee. Same applies to effective range. If it isn't a straight path, we will
	// rethink too soon, which is okay.
	Vector2F moveToRange;		// Destination of move (uses final destination).
	float    moveToTime = 1.0f;	// Seconds to the desired location is reached.

	// Consider flocking. This wasn't really working in a combat situation.
	// May reconsider later, or just use for spreading out.
	static  float FLOCK_MOVE_BIAS = 0.2f;
	Vector2F heading = thisComp.spatial->GetHeading2D();
	Vector2F flockDir = heading;
	utility[OPTION_FLOCK_MOVE] = 0.001f;

	for( int k=0; k<enemyList.Size(); ++k ) {

		ComponentSet enemy( GetChitBag()->GetChit(enemyList[k]), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
		if ( !enemy.okay ) {
			continue;
		}
		const Vector3F	enemyPos		= enemy.spatial->GetPosition();
		const Vector2F	enemyPos2		= { enemyPos.x, enemyPos.z };
		float			range			= (enemyPos - pos).Length();
		Vector3F		toEnemy			= (enemyPos - pos);
		Vector2F		normalToEnemy	= { toEnemy.x, toEnemy.z };
		float			meleeRange		= BattleMechanics::MeleeRange( parentChit, enemy.chit );

		normalToEnemy.Normalize();
		float dot = DotProduct( normalToEnemy, heading );

		// Prefer targets we are pointed at.
		static const float DOT_BIAS = 0.25f;
		float q = 1.0f + dot * DOT_BIAS;
		// Prefer the current target & focused target.
		if ( enemyList[k] == currentTarget ) {
			q *= 2;
			if ( focusOnTarget ) {
				q *= 2;
			}
		}

		// Consider ranged weapon options: OPTION_SHOOT, OPTION_MOVE_TO_RANGE
		if ( rangedWeapon ) {
			GameItem* pw = rangedWeapon->GetItem();
			float radAt1 = BattleMechanics::ComputeRadAt1(	thisComp.chit->GetItem(),
															rangedWeapon,
															false,	// SHOOT implies stopping.
															enemy.move && enemy.move->IsMoving() );
			
			float effectiveRange = BattleMechanics::EffectiveRange( radAt1 );

			// 1.5f gives spacing for bolt to start.
			// The HasRound() && !Reloading() is really important: if the gun
			// is in cooldown, don't give up on shooting and do something else!
			if ( pw->HasRound() && !pw->Reloading() && range > 1.5f ) {
				float u = 1.0f - (range - effectiveRange) / effectiveRange; 
				u *= q;

				// This needs tuning.
				// If the unit has been shooting, time to do something else.
				// Stand around and shoot battles are boring and look crazy.
				if ( currentAction == SHOOT ) {
					u *= 0.5f;
				} 

				// Don't blow ourself up:
				if ( pw->flags & GameItem::EFFECT_EXPLOSIVE ) {
					if ( range < EXPLOSIVE_RANGE * 2.0f ) {
						u *= 0.1f;	// blowing ourseves up is bad.
					}
				}

				if ( u > utility[OPTION_SHOOT] && LineOfSight( thisComp, enemy.chit ) ) {
					utility[OPTION_SHOOT] = u;
					target[OPTION_SHOOT] = enemy.chit;
				}
			}
			// Move to the effective range?
			float u = ( range - effectiveRange ) / effectiveRange;
			u *= q;
			if ( pw->CanUse() ) {
				// okay;
			}
			else {
				// Moving to effective range is less interesting if the gun isn't ready.
				u *= 0.5f;
			}
			if ( u > utility[OPTION_MOVE_TO_RANGE] ) {
				utility[OPTION_MOVE_TO_RANGE] = u;
				moveToRange = enemyPos2;//pos2 + normalToEnemy * (range - effectiveRange);
				moveToTime  = (range - effectiveRange ) / pmc->Speed();
				target[OPTION_MOVE_TO_RANGE] = enemy.chit;
			}
		}

		// Consider Melee
		if ( meleeWeapon ) {
			float u = meleeRange / range;
			u *= q;
			// Utility of the actual charge vs. moving closer. This
			// seems to work with an if case.
			if ( range > meleeRange * 3.0f ) {
				if ( u > utility[OPTION_MOVE_TO_RANGE] ) {
					utility[OPTION_MOVE_TO_RANGE] = u;
					//moveToRange = pos2 + normalToEnemy * (range - meleeRange*2.0f);
					moveToRange = enemyPos2;
					moveToTime = (range - meleeRange*2.0f) / pmc->Speed();
					target[OPTION_MOVE_TO_RANGE] = enemy.chit;
				}
			}
			u *= 0.95f;	// a little less utility than the move_to_range
			if ( u > utility[OPTION_MELEE] ) {
				utility[OPTION_MELEE] = u;
				target[OPTION_MELEE] = enemy.chit;
			}
		}
	}

	int index = MaxValue<float, CompValue>( utility, NUM_OPTIONS );
	rethink.Reset();

	// Translate to action system:
	switch ( index ) {
		case OPTION_FLOCK_MOVE:
		{
			currentAction = MOVE;
			Vector2F dest = pos2 + flockDir;
			pmc->QueueDest( dest );
		}
		break;

		case OPTION_MOVE_TO_RANGE:
		{
			currentAction = MOVE;
			currentTarget = target[OPTION_MOVE_TO_RANGE]->ID();
			rethink.Within( (U32)(moveToTime*1000.0f) );
			pmc->QueueDest( moveToRange );
		}
		break;

		case OPTION_MELEE:
		{
			currentAction = MELEE;
			currentTarget = target[OPTION_MELEE]->ID();
		}
		break;
		
		case OPTION_SHOOT:
		{
			pmc->Stop();
			currentAction = SHOOT;
			currentTarget = target[OPTION_SHOOT]->ID();
		}
		break;

		default:
			GLASSERT( 0 );
	};

#ifdef AI_OUTPUT
	if ( debugFlag ) {
		static const char* optionName[NUM_OPTIONS] = { "flock", "mtrange", "melee", "shoot" };
		GLOUTPUT(( "ID=%d Think: flock=%.2f mtrange=%.2f melee=%.2f shoot=%.2f -> %s\n",
				   thisComp.chit->ID(), utility[OPTION_FLOCK_MOVE], utility[OPTION_MOVE_TO_RANGE], utility[OPTION_MELEE], utility[OPTION_SHOOT],
				   optionName[index] ));
	}
#endif
}


int AIComponent::DoTick( U32 deltaTime, U32 timeSince )
{
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   Chit::MOVE_BIT |
									   Chit::ITEM_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay ) {
		return ABOUT_1_SEC;
	}

	// If we are in some action, do nothing and return.
	if (    parentChit->GetRenderComponent() 
		 && !parentChit->GetRenderComponent()->AnimationReady() ) 
	{
		return 0;
	}

	wanderTime += timeSince;

	ChitBag* chitBag = this->GetChitBag();

	// Focuesd move check
	if ( focusedMove ) {
		return 200;
	}

	// If focused, make sure we have a target.
	if ( currentTarget && !chitBag->GetChit( currentTarget )) {
		currentTarget = 0;
	}
	if ( !currentTarget ) {
		focusOnTarget = false;
	}

	GetFriendEnemyLists();

	// High level mode switch?
	if ( aiMode == NORMAL_MODE && !enemyList.Empty() ) {
		aiMode = BATTLE_MODE;
		currentAction = 0;
#ifdef AI_OUTPUT
		if ( debugFlag ) {
			GLOUTPUT(( "ID=%d Mode to Battle\n", thisComp.chit->ID() ));
		}
#endif
	}
	else if ( aiMode == BATTLE_MODE && currentTarget == 0 && enemyList.Empty() ) {
		aiMode = NORMAL_MODE;
		currentAction = 0;
#ifdef AI_OUTPUT
		if ( debugFlag ) {
			GLOUTPUT(( "ID=%d Mode to Normal\n", thisComp.chit->ID() ));
		}
#endif
	}

	if ( !currentAction || rethink.Delta(timeSince)) {
		Think( thisComp );
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	int tick = 0;
	switch( currentAction ) {

		case MOVE:		
			DoMove( thisComp );
			tick = 0;
			break;
		case MELEE:		
			DoMelee( thisComp );	
			tick = 0;
			break;
		case SHOOT:		
			DoShoot( thisComp );
			tick = 0;
			break;
		case NO_ACTION:
			tick = 400;
			break;

		case WANDER:
			tick = VERY_LONG_TICK;
			break;

		default:
			GLASSERT( 0 );
			currentAction = 0;
			break;
	}
	return Min( tick, rethink.Next() );
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	switch ( msg.ID() ) {
	case ChitMsg::PATHMOVE_DESTINATION_REACHED:
		if ( currentAction != WANDER ) {
			focusedMove = false;
			currentAction = NO_ACTION;
			parentChit->SetTickNeeded();
		}
		else {
			randomWander = false;
		}
		break;

	case ChitMsg::PATHMOVE_DESTINATION_BLOCKED:
		if ( currentAction != WANDER ) {
			focusedMove = false;
			currentAction = NO_ACTION;
			parentChit->SetTickNeeded();
		}
		else {
			randomWander = true;
		}
		break;

	default:
		super::OnChitMsg( chit, msg );
		break;
	}
}


