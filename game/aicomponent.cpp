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
#include "gridmovecomponent.h"
#include "sectorport.h"
#include "workqueue.h"
#include "team.h"

#include "../script/battlemechanics.h"
#include "../script/plantscript.h"
#include "../script/worldgen.h"
#include "../script/corescript.h"
#include "../script/itemscript.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

#include "../grinliz/glrectangle.h"
//#include "../grinliz/glperformance.h"
#include "../Shiny/include/Shiny.h"
#include <climits>

using namespace grinliz;

static const float	NORMAL_AWARENESS			= 10.0f;
static const float	LOOSE_AWARENESS				= 16.0f;
static const float	SHOOT_ANGLE					= 10.0f;	// variation from heading that we can shoot
static const float	SHOOT_ANGLE_DOT				=  0.985f;	// same number, as dot product.
static const float	WANDER_RADIUS				=  5.0f;
static const float	EAT_HP_PER_SEC				=  2.0f;
static const float	EAT_HP_HEAL_MULT			=  5.0f;	// eating really tears up plants. heal the critter more than damage the plant.
static const float  CORE_HP_PER_SEC				=  8.0f;
static const int	WANDER_ODDS					=100;		// as in 1 in WANDER_ODDS
static const int	GREATER_WANDER_ODDS			=  5;		// as in 1 in WANDER_ODDS
static const float	PLANT_AWARE					=  3.0f;
static const float	GOLD_AWARE					=  3.5f;
static const int	FORCE_COUNT_STUCK			=  8;
static const int	STAND_TIME_WHEN_WANDERING	= 1500;

const char* AIComponent::MODE_NAMES[NUM_MODES]     = { "normal", "rockbreak", "battle" };
const char* AIComponent::ACTION_NAMES[NUM_ACTIONS] = { "none", "move", "melee", "shoot", "wander", "stand" };


AIComponent::AIComponent( Engine* _engine, WorldMap* _map )
{
	engine = _engine;
	map = _map;
	currentAction = 0;
	focus = 0;
	friendEnemyAge = 0;
	aiMode = NORMAL_MODE;
	wanderTime = 0;
	rethink = 0;
	fullSectorAware = false;
	debugFlag = false;
	visitorIndex = -1;
	playerControlled = false;
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
	XARC_SER( xs, targetDesc.id );
	XARC_SER( xs, targetDesc.mapPos );
	XARC_SER( xs, focus );
	XARC_SER( xs, wanderTime );
	XARC_SER( xs, rethink );
	XARC_SER( xs, fullSectorAware );
	XARC_SER( xs, friendEnemyAge );
	XARC_SER( xs, visitorIndex );
	this->EndSerialize( xs );
}


void AIComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
}


void AIComponent::OnRemove()
{
	super::OnRemove();
}


int AIComponent::GetTeamStatus( Chit* other )
{
	// FIXME: placeholder friend/enemy logic
	GameItem* thisItem  = parentChit->GetItem();
	GameItem* otherItem = other->GetItem();

	int t0  = thisItem ? thisItem->primaryTeam  : 0;
	int t1 = otherItem ? otherItem->primaryTeam : 0;

	return GetRelationship( t0, t1 );
}


bool AIComponent::LineOfSight( const ComponentSet& thisComp, Chit* t )
{

	Vector3F origin, dest;
	IRangedWeaponItem* weapon = thisComp.itemComponent->GetRangedWeapon( &origin );
	GLASSERT( weapon );

	ComponentSet target( t, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	if ( !target.okay ) {
		return false;
	}
	target.render->GetMetaData( META_TARGET, &dest );

	Vector3F dir = dest - origin;
	float length = dir.Length() + 0.01f;	// a little extra just in case
	CArray<const Model*, RenderComponent::NUM_MODELS+1> ignore, targetModels;
	thisComp.render->GetModelList( &ignore );

	ModelVoxel mv = engine->IntersectModelVoxel( origin, dir, length, TEST_TRI, 0, 0, ignore.Mem() );
	if ( mv.model ) {
		target.render->GetModelList( &targetModels );
		return targetModels.Find( mv.model ) >= 0;
	}
	return false;
}


bool AIComponent::LineOfSight( const ComponentSet& thisComp, const grinliz::Vector2I& mapPos )
{
	Vector3F origin;
	IRangedWeaponItem* weapon = thisComp.itemComponent->GetRangedWeapon( &origin );
	GLASSERT( weapon );
	CArray<const Model*, RenderComponent::NUM_MODELS+1> ignore;
	thisComp.render->GetModelList( &ignore );

	Vector3F dest = { (float)mapPos.x+0.5f, 0.5f, (float)mapPos.y+0.5f };
	Vector3F dir = dest - origin;
	float length = dir.Length() + 0.01f;	// a little extra just in case

	ModelVoxel mv = engine->IntersectModelVoxel( origin, dir, length, TEST_TRI, 0, 0, ignore.Mem() );

	// A little tricky; we hit the 'mapPos' if nothing is hit (which gets to the center)
	// or if voxel at that pos is hit.
	if ( !mv.Hit() || ( mv.Hit() && mv.Voxel2() == mapPos )) {
		return true;
	}
	return false;
}


void AIComponent::GetFriendEnemyLists()
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return;
	Vector2F center = sc->GetPosition2D();

	Rectangle2F zone;
	zone.min = zone.max = center;
	zone.Outset( fullSectorAware ? SECTOR_SIZE : NORMAL_AWARENESS );

	if ( map->UsingSectors() ) {
		Rectangle2I ri = SectorData::InnerSectorBounds( center.x, center.y );
		Rectangle2F rf;
		rf.Set( (float)ri.min.x, (float)ri.min.y, (float)ri.max.x, (float)ri.max.y );

		zone.DoIntersection( rf );
	}

	friendList.Clear();
	enemyList.Clear();

	CChitArray chitArr;
	MoBFilter mobFilter;

	GetChitBag()->QuerySpatialHash( &chitArr, zone, parentChit, &mobFilter );
	for( int i=0; i<chitArr.Size(); ++i ) {
		int status = GetTeamStatus( chitArr[i] );
		if ( status == RELATE_ENEMY ) {
			if (    enemyList.HasCap() 
				 && ( fullSectorAware || map->HasStraightPath( center, chitArr[i]->GetSpatialComponent()->GetPosition2D() ))) 
			{
				enemyList.Push( chitArr[i]->ID());
			}
		}
		else if ( status == RELATE_FRIEND ) {
			if ( friendList.HasCap() ) {
				friendList.Push( chitArr[i]->ID());
			}
		}
	}

	// Add the currentTarget back in, if we lost it. But only if
	// it hasn't gone too far away.
	Chit* currentTargetChit = GetChitBag()->GetChit( targetDesc.id );
	if ( currentTargetChit && currentTargetChit->GetSpatialComponent() ) {
		Vector2F targetCenter = currentTargetChit->GetSpatialComponent()->GetPosition2D();
		if (    (targetCenter - center).LengthSquared() < LOOSE_AWARENESS*LOOSE_AWARENESS		// close enough OR
			 || (focus == FOCUS_TARGET) )														// focused
		{
			int i = enemyList.Find( targetDesc.id );
			if ( i < 0 && enemyList.HasCap() ) {
				enemyList.Push( targetDesc.id );
			}
		}
	}
}


class ChitDistanceCompare : public ISortCompare<Chit*>
{
public:
	ChitDistanceCompare( const Vector3F& _origin ) : origin(_origin) {}
	virtual bool Less( Chit* v0, Chit* v1 )
	{
		return ( v0->GetSpatialComponent()->GetPosition() - origin ).LengthSquared() <
			   ( v1->GetSpatialComponent()->GetPosition() - origin ).LengthSquared();
	}

private:
	Vector3F origin;
};

Chit* AIComponent::Closest( const ComponentSet& thisComp, Chit* arr[], int n, Vector2F* outPos, float* distance )
{
	float best = FLT_MAX;
	Chit* chit = 0;
	Vector3F pos = thisComp.spatial->GetPosition();

	for( int i=0; i<n; ++i ) {
		Chit* c = arr[i];
		SpatialComponent* sc = c->GetSpatialComponent();
		if ( sc ) {
			float len2 = (sc->GetPosition() - pos).LengthSquared();
			if ( len2 < best ) {
				best = len2;
				chit = c;
			}
		}
	}
	if ( distance ) {
		*distance = chit ? sqrtf( best ) : 0;
	}
	if ( outPos ) {
		if ( chit ) {
			*outPos = chit->GetSpatialComponent()->GetPosition2D();
		}
		else {
			outPos->Zero();
		}
	}
	return chit;
}


void AIComponent::DoMove( const ComponentSet& thisComp )
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( !pmc || pmc->ForceCountHigh() || pmc->Stopped() ) {
		currentAction = NO_ACTION;
		return;
	}

	// Generally speaking, moving is done by the PathMoveComponent. When
	// not in battle, this is essentially "do nothing." If in battle mode,
	// we look for opportunity fire and such.
	if ( aiMode != BATTLE_MODE ) {
		// Check for motion done, stuck, etc.
		if ( pmc->Stopped() || pmc->ForceCountHigh() ) {
			currentAction = NO_ACTION;
			return;
		}
		// Reloading is always a good background task.
		IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
		if ( rangedWeapon && rangedWeapon->GetItem()->CanReload() ) {
			rangedWeapon->GetItem()->Reload( parentChit );
		}
	}
	else {
		// Battle mode! Run & Gun
		float utilityRunAndGun = 0.0f;
		Chit* targetRunAndGun = 0;

		IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
		GameItem* ranged = rangedWeapon ? rangedWeapon->GetItem() : 0;

		if ( ranged ) {
			Vector3F heading = thisComp.spatial->GetHeading();
			bool explosive = (ranged->GetItem()->flags & GameItem::EFFECT_EXPLOSIVE) != 0;

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

					if ( explosive && range < EXPLOSIVE_RANGE*3.0f ) {
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
				if ( debugFlag ) {
					GLOUTPUT(( "ID=%d Move: RunAndGun=%.2f Reload=%.2f ", thisComp.chit->ID(), utilityRunAndGun, utilityReload ));
				}
				if ( utilityRunAndGun > utilityReload ) {
					GLASSERT( targetRunAndGun );
					Vector3F leading = BattleMechanics::ComputeLeadingShot( thisComp.chit, targetRunAndGun, 0 );
					BattleMechanics::Shoot(	GetChitBag(), 
											thisComp.chit, 
											leading, 
											targetRunAndGun->GetMoveComponent() ? targetRunAndGun->GetMoveComponent()->IsMoving() : false,
											ranged->ToRangedWeapon() );
					if ( debugFlag ) {
						GLOUTPUT(( "->RunAndGun\n" ));
					}
				}
				else {
					ranged->Reload( parentChit );
					if ( debugFlag ) {
						GLOUTPUT(( "->Reload\n" ));
					}
				}
			}
		}
	}
}


void AIComponent::DoShoot( const ComponentSet& thisComp )
{
	bool pointed = false;
	Vector3F leading = { 0, 0, 0 };
	bool isMoving = false;

	if ( targetDesc.id ) {
		ComponentSet target( GetChitBag()->GetChit( targetDesc.id ), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
		if ( !target.okay ) {
			currentAction = NO_ACTION;
			return;
		}

		leading = BattleMechanics::ComputeLeadingShot( thisComp.chit, target.chit, 0 );
		isMoving = target.chit->GetMoveComponent() ? target.chit->GetMoveComponent()->IsMoving() : false;
	}
	else if ( !targetDesc.mapPos.IsZero() ) {
		leading.Set( (float)targetDesc.mapPos.x + 0.5f, 0.5f, (float)targetDesc.mapPos.y + 0.5f );
	}
	else {
		// case not supposed to happen, but never seen it be harmful:
		currentAction = 0;
		return;
	}
	Vector2F leading2D = { leading.x, leading.z };
	// Rotate to target.
	Vector2F heading = thisComp.spatial->GetHeading2D();
	float headingAngle = RotationXZDegrees( heading.x, heading.y );

	Vector2F normalToTarget = leading2D - thisComp.spatial->GetPosition2D();
	float distanceToTarget = normalToTarget.Length();
	normalToTarget.Normalize();
	float angleToTarget = RotationXZDegrees( normalToTarget.x, normalToTarget.y );
	float deltaAngle = MinDeltaDegrees( headingAngle, angleToTarget, 0 );

	if ( deltaAngle < SHOOT_ANGLE ) {
		// all good.
	}
	else {
		// Rotate to target.
		PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
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
				BattleMechanics::Shoot(	GetChitBag(), 
										parentChit, 
										leading,
										isMoving,
										weapon );
			}
		}
		else {
			item->Reload( parentChit );
			// Out of ammo - do something else.
			currentAction = NO_ACTION;
		}
	}
}


void AIComponent::DoMelee( const ComponentSet& thisComp )
{
	IMeleeWeaponItem* weapon = thisComp.itemComponent->GetMeleeWeapon();
	ComponentSet target( GetChitBag()->GetChit( targetDesc.id ), Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );

	bool targetOkay = false;
	if ( targetDesc.id ) {
		targetOkay = target.okay;
	}
	else if ( !targetDesc.mapPos.IsZero() ) {
		// make sure we aren't swinging at an empty voxel.
		targetOkay = map->GetWorldGrid( targetDesc.mapPos.x, targetDesc.mapPos.y ).RockHeight() > 0;
	}

	if ( !weapon || !targetOkay ) {
		currentAction = NO_ACTION;
		return;
	}
	GameItem* item = weapon->GetItem();

	// Are we close enough to hit? Then swing. Else move to target.
	if ( targetDesc.id && BattleMechanics::InMeleeZone( engine, parentChit, target.chit )) {
		GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
	}
	else if ( !targetDesc.id && BattleMechanics::InMeleeZone( engine, parentChit, targetDesc.mapPos )) {
		GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
	}
	else {
		// Move to target.
		PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
		if ( pmc ) {
			Vector2F targetPos = { 0, 0 };
			if ( targetDesc.id )
				targetPos = target.spatial->GetPosition2D();
			else 
				targetPos.Set( (float)targetDesc.mapPos.x + 0.5f, (float)targetDesc.mapPos.y + 0.5f );

			Vector2F pos = thisComp.spatial->GetPosition2D();
			Vector2F dest = { -1, -1 };
			pmc->QueuedDest( &dest );

			float delta = ( dest-targetPos ).Length();
			float distanceToTarget = (targetPos - pos).Length();

			// If the error is greater than distance to, then re-path.
			if ( delta > distanceToTarget * 0.25f ) {
				pmc->QueueDest( targetPos );
			}
		}
		else {
			currentAction = NO_ACTION;
			return;
		}
	}
}


bool AIComponent::DoStand( const ComponentSet& thisComp, U32 since )
{
	const GameItem* item	= parentChit->GetItem();
	int itemFlags			= item ? item->flags : 0;
	float totalHP			= item ? item->TotalHPF() : 0;
	int tick = 400;

	// Plant eater
	if (	!thisComp.move->IsMoving()    
		 && (item->hp < totalHP ))  
	{

		if ( itemFlags & GameItem::AI_EAT_PLANTS ) {
			// Are we on a plant?
			Vector2F pos = thisComp.spatial->GetPosition2D();
			// Note that currently only support eating stage 0-1 plants.
			CChitArray plants;
			PlantFilter plantFilter( -1, MAX_PASSABLE_PLANT_STAGE );

			parentChit->GetChitBag()->QuerySpatialHash( &plants, pos, 0.4f, 0, &plantFilter );
			if ( !plants.Empty() ) {
				// We are standing on a plant.
				float hp = Travel( EAT_HP_PER_SEC, since );
				ChitMsg heal( ChitMsg::CHIT_HEAL );
				heal.dataF = hp * EAT_HP_HEAL_MULT;

				if ( debugFlag ) {
					GLOUTPUT(( "ID=%d Eating plants itemHp=%.1f total=%.1f hp=%.1f\n", thisComp.chit->ID(),
						item->hp, item->TotalHP(), hp ));
				}

				DamageDesc dd( hp, 0 );
				ChitDamageInfo info( dd );

				info.originID = parentChit->ID();
				info.awardXP  = false;
				info.isMelee  = true;
				info.isExplosion = false;
				info.originOfImpact = thisComp.spatial->GetPosition();

				ChitMsg damage( ChitMsg::CHIT_DAMAGE, 0, &info );
				parentChit->SendMessage( heal, this );
				plants[0]->SendMessage( damage );
				return true;
			}
		}
		if ( itemFlags & GameItem::AI_HEAL_AT_CORE ) {
			// Are we on a core?
			Vector2I pos2i = thisComp.spatial->GetPosition2DI();
			Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
			const SectorData& sd = map->GetSector( sector );
			if ( sd.core == pos2i ) {
				ChitMsg heal( ChitMsg::CHIT_HEAL );
				heal.dataF = Travel( CORE_HP_PER_SEC, since );
				parentChit->SendMessage( heal, this );
				return true;
			}
		}
	}
	
	if (    visitorIndex >= 0
		      && !thisComp.move->IsMoving() )
	{
		// Visitors at a kiosk.
		Vector2I pos2i = thisComp.spatial->GetPosition2DI();
		Chit* chit = this->GetChitBag()->ToLumos()->QueryPorch( pos2i );
		
		VisitorData* vd = &Visitors::Instance()->visitorData[visitorIndex];
		IString kioskName = vd->CurrentKioskWant();

		if ( chit && chit->GetItem()->name == kioskName ) {
			vd->kioskTime += since;
			if ( vd->kioskTime > VisitorData::KIOSK_TIME ) {
				Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
				vd->DidVisitKiosk( sector );
				vd->kioskTime = 0;
				currentAction = NO_ACTION;	// done here - move on!
				return false;
			}
			// else keep standing.
			return true;
		}
		else {
			// Oops...
			currentAction = NO_ACTION;
			return false;
		}
	}
	
	if ( !taskList.Empty() && taskList[0].action == STAND ) {
		const Vector2I& pos2i = taskList[0].pos2i;
		Vector3F pos = { (float)pos2i.x+0.5f, 0.0f, (float)pos2i.y+0.5f };
		engine->particleSystem->EmitPD( "construction", pos, V3F_UP, 30 );	// FIXME: standard delta constant
		taskList[0].timer -= since;
		return true;	// keep standing
	}
	return false;
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

	super::OnChitEvent( event );
}


void AIComponent::Think( const ComponentSet& thisComp )
{
	switch( aiMode ) {
	case NORMAL_MODE:
		if ( visitorIndex >= 0 )
			ThinkVisitor( thisComp );
		else
			ThinkWander( thisComp );	
		break;
	case ROCKBREAK_MODE:	ThinkRockBreak( thisComp );	break;
	case BATTLE_MODE:		ThinkBattle( thisComp );	break;
	}
};



bool AIComponent::Move( const SectorPort& sp, bool focused )
{
	PathMoveComponent* pmc    = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	SpatialComponent*  sc	  = parentChit->GetSpatialComponent();
	if ( pmc && sc ) {
		// Read our destination port information:
		const SectorData& sd = map->GetSector( sp.sector );
				
		// Read our local get-on-the-grid info
		SectorPort local = map->NearestPort( sc->GetPosition2D() );
		// Completely possible this chit can't actually path anywhere.
		if ( local.IsValid() ) {
			const SectorData& localSD = map->GetSector( local.sector );
			// Local path to remote dst
			Vector2F dest2 = SectorData::PortPos( localSD.GetPortLoc(local.port), parentChit->ID() );
			pmc->QueueDest( dest2, -1, &sp );
			currentAction = MOVE;
			focus = focused ? FOCUS_MOVE : 0;
			return true;
		}
	}
	return false;
}


void AIComponent::Pickup( Chit* item )
{
	taskList.Push( Task::MoveTask( item->GetSpatialComponent()->GetPosition2D(), 0 ));
	taskList.Push( Task::PickupTask( item->ID(), 0 ));
}


void AIComponent::Move( const grinliz::Vector2F& dest, bool focused, float rotation )
{
	PathMoveComponent* pmc    = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( pmc ) {
		pmc->QueueDest( dest, rotation );
		currentAction = MOVE;
		focus = focused ? FOCUS_MOVE : 0;
	}
}


void AIComponent::Target( Chit* chit, bool focused )
{
	aiMode = BATTLE_MODE;
	targetDesc.Set( chit->ID() );
	focus = focused ? FOCUS_TARGET : 0;
}


bool AIComponent::RockBreak( const grinliz::Vector2I& rock )
{
	GLOUTPUT(( "Destroy something at %d,%d\n", rock.x, rock.y ));
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay ) 
		return false;

	Vector2I sector = thisComp.spatial->GetPosition2DI();
	sector.x /= SECTOR_SIZE;
	sector.y /= SECTOR_SIZE;

	CoreScript* cs = GetLumosChitBag()->GetCore( sector );
	if ( cs && cs->GetAttached(0) && (cs->GetAttached(0) != parentChit) ) {
		// Core is in use. We can only blast away.
		const WorldGrid& wg = map->GetWorldGrid( rock.x, rock.y );
		if ( wg.RockHeight() == 0 )
			return false;

		aiMode = ROCKBREAK_MODE;
		currentAction = NO_ACTION;
		targetDesc.Set( rock );
		GLASSERT( targetDesc.HasTarget() );
		parentChit->SetTickNeeded();
	}
	else {
		// neutral. Can vaporize.
		Vector2F dest = { (float)rock.x + 0.5f, (float)rock.y+0.5f };
		Vector2F end = { 0, 0 };
		float cost = 0;
		if ( map->CalcPathBeside( thisComp.spatial->GetPosition2D(), dest, &end, &cost )) {
			taskList.Clear();
			taskList.Push( Task::MoveTask( end, 0 ));
			taskList.Push( Task::StandTask( 1000, 0 ));
			taskList.Push( Task::RemoveTask( rock, 0 ));
		}
	}
	return true;
}


WorkQueue* AIComponent::GetWorkQueue()
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc )
		return 0;

	Vector2I sector = sc->GetSector();
	CoreScript* coreScript	= GetChitBag()->ToLumos()->GetCore( sector );
	if ( !coreScript )
		return 0;

	WorkQueue* workQueue = coreScript->GetWorkQueue();
	return workQueue;
}

	
void AIComponent::ThinkRockBreak( const ComponentSet& thisComp )
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	const WorldGrid& wg = map->GetWorldGrid( targetDesc.mapPos.x, targetDesc.mapPos.y );

	const Vector3F& pos = thisComp.spatial->GetPosition();
	Vector2F pos2		= { pos.x, pos.z };
	Vector2I pos2i		= { (int)pos2.x, (int)pos2.y };
	Vector3F rockTarget	= targetDesc.MapTarget();
	Vector2I rock2i		= { (int)rockTarget.x, (int)rockTarget.z };
	Vector2F rock2		= { (float)rock2i.x + 0.5f, (float)rock2i.y + 0.5f };
	
	if ( !pmc || wg.RockHeight() == 0 ) {
		currentAction	= NO_ACTION;
		aiMode			= NORMAL_MODE;
		return;
	}

	// The current weapons.
	IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
	IMeleeWeaponItem*  meleeWeapon  = thisComp.itemComponent->GetMeleeWeapon();

	// Always use melee first because bolts tend to "shoot through" in close quarters.
	if ( meleeWeapon && BattleMechanics::InMeleeZone( engine, thisComp.chit, rock2i )) {
		currentAction = MELEE;
		return;
	}
	bool lineOfSight = rangedWeapon && LineOfSight( thisComp, targetDesc.mapPos );
	if ( lineOfSight && rangedWeapon->GetItem()->CanUse() ) {
		GLASSERT( targetDesc.HasTarget() );
		currentAction = SHOOT;
		return;
	}
	else if ( !lineOfSight || meleeWeapon ) {
		// Move to target
		Vector2F dest = { (float)targetDesc.mapPos.x + 0.5f, (float)targetDesc.mapPos.y + 0.5f };
		Vector2F end;
		float cost = 0;
		if ( map->CalcPathBeside( thisComp.spatial->GetPosition2D(), dest, &end, &cost )) {
			this->Move( end, false );
			return;
		}
	}
	aiMode = NORMAL_MODE;
	currentAction = NO_ACTION;
}


Vector2F AIComponent::GetWanderOrigin( const ComponentSet& thisComp ) const
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
	Vector2F dest = GetWanderOrigin( thisComp );
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

	dest = GetWanderOrigin( thisComp ) + v;
	return dest;
}


Vector2F AIComponent::ThinkWanderRandom( const ComponentSet& thisComp )
{
	if ( thisComp.item->flags & GameItem::AI_DOES_WORK ) {
		// workers stay close to core.
		Vector2F dest = GetWanderOrigin( thisComp );
		dest.x += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
		dest.y += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
		return dest;
	}

	Vector2I pos2i = thisComp.spatial->GetPosition2DI();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };

	Vector2F dest = { 0, 0 };
	dest.x = (float)(sector.x*SECTOR_SIZE + 1 + parentChit->random.Rand( SECTOR_SIZE-2 )) + 0.5f;
	dest.y = (float)(sector.y*SECTOR_SIZE + 1 + parentChit->random.Rand( SECTOR_SIZE-2 )) + 0.5f;
	return dest;
}


Vector2F AIComponent::ThinkWanderFlock( const ComponentSet& thisComp )
{
	Vector2F origin = thisComp.spatial->GetPosition2D();

	static const int NPLANTS = 4;
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
	// Only consider the wander origin for workers.
	if ( thisComp.item->flags & GameItem::AI_DOES_WORK ) {
		pos.Push( GetWanderOrigin( thisComp ) );	// the origin is a friend.
	}

	// And plants are friends.
	Rectangle2F r;
	r.min = r.max = origin;
	r.Outset( PLANT_AWARE );

	CChitArray plants;
	PlantFilter plantFilter( -1, MAX_PASSABLE_PLANT_STAGE );
	parentChit->GetChitBag()->QuerySpatialHash( &plants, r, 0, &plantFilter );
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
	// FIXME: why is this 2-pass? Left over code?
	for( int pass=0; pass<2; ++pass ) {
		for( int i=0; i<pos.Size(); ++i ) {
			if ( (pos[i] - dest).LengthSquared() < TOO_CLOSE*TOO_CLOSE ) {
				dest += heading * TOO_CLOSE;
			}
		}
	}
	return dest;
}


bool AIComponent::SectorHerd( const ComponentSet& thisComp )
{
	static const int NDELTA = 8;
	Vector2I delta[NDELTA] = { 
		{-1,0}, {1,0}, {0,-1}, {0,1},
		{-1,-1}, {1,-1}, {-1,1}, {1,1}
	};
	parentChit->random.ShuffleArray( delta, NDELTA );
	Vector2F pos = thisComp.spatial->GetPosition2D();

	SectorPort start = map->NearestPort( pos );
	if ( start.IsValid() ) {
		for( int i=0; i<NDELTA; ++i ) {
			SectorPort dest;
			dest.sector = start.sector + delta[i];
			const SectorData& destSD = map->GetSector( dest.sector );
			if ( destSD.ports ) {
				dest.port = destSD.NearestPort( pos );

				RenderComponent* rc = parentChit->GetRenderComponent();
				if ( rc ) {
					rc->Deco( "horn", RenderComponent::DECO_HEAD, 10*1000 );
				}
				NewsEvent news( NewsEvent::PONY, pos, StringPool::Intern( "SectorHerd" ), parentChit->ID() );
				GetChitBag()->AddNews( news );

				ChitMsg msg( ChitMsg::CHIT_SECTOR_HERD, 0, &dest );
				for( int i=0; i<friendList.Size(); ++i ) {
					Chit* c = GetChitBag()->GetChit( friendList[i] );
					if ( c ) {
						c->SendMessage( msg );
					}
				}
				parentChit->SendMessage( msg );
				return true;
			}
		}
	}
	return false;
}


void AIComponent::ThinkVisitor( const ComponentSet& thisComp )
{
	// Visitors can:
	// - go to kiosks, stand, then move on to a different domain
	// - disconnect
	// - grid travel to a new domain

	if ( !thisComp.okay ) return;

	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( !pmc ) return;											// strange state,  can't do anything
	if ( !pmc->Stopped() && !pmc->ForceCountHigh() ) return;	// everything is okay. move along.

	bool disconnect = false;

	Vector2I pos2i = thisComp.spatial->GetPosition2DI();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	VisitorData* vd = Visitors::Get( visitorIndex );
	Chit* kiosk = GetChitBag()->ToLumos()->QueryPorch( pos2i );
	IString kioskName = vd->CurrentKioskWant();
	if ( kiosk && kiosk->GetItem()->name == kioskName ) {
		// all good
	}
	else {
		kiosk = 0;
	}

	if (    vd->nVisits >= VisitorData::MAX_VISITS
		 || vd->nWants  >= VisitorData::NUM_VISITS ) 
	{
		disconnect = true;
		if ( debugFlag ) {
			GLOUTPUT(( "ID=%d Visitor travel done.\n", thisComp.chit->ID() ));
		}
	}
	else {
		// Move on,
		// Stand, or
		// Move to a kiosk
		if ( vd->doneWith == sector ) {
			// Head out!
			SectorPort sp = Visitors::Instance()->ChooseDestination( visitorIndex, map, GetChitBag()->ToLumos() );
			bool okay = this->Move( sp, true );
			if ( !okay ) disconnect = true;
		}
		else if ( pmc->Stopped() && kiosk ) {
			currentAction = STAND;
		}
		else {
			// Find a kiosk.
			CChitArray arr;
			Rectangle2I inneri = map->GetSector( sector ).InnerBounds();
			Rectangle2F inner;
			inner.Set( (float)inneri.min.x, (float)inneri.min.y, (float)(inneri.max.x+1), (float)(inneri.max.y+1) );

			ItemNameFilter kioskFilter( vd->CurrentKioskWant() );
			parentChit->GetChitBag()->QuerySpatialHash( &arr, inner, 0, &kioskFilter );

			if ( arr.Empty() ) {
				// Done here.
				vd->NoKiosk( sector );
				if ( debugFlag ) {
					GLOUTPUT(( "ID=%d Visitor: no kiosk.\n", thisComp.chit->ID() ));
				}
			}
			else {
				// Random!
				parentChit->random.ShuffleArray( arr.Mem(), arr.Size() );
				bool found = false;

				for( int i=0; i<arr.Size(); ++i ) {
					Chit* kiosk = arr[i];
					MapSpatialComponent* msc = GET_SUB_COMPONENT( kiosk, SpatialComponent, MapSpatialComponent );
					GLASSERT( msc );
					Vector2I porchi = msc->PorchPos();
					Vector2F porch = { (float)porchi.x+0.5f, (float)porchi.y+0.5f };
					if ( map->CalcPath( thisComp.spatial->GetPosition2D(), porch, 0, 0, 0, 0 ) ) {
						this->Move( porch, false );
						found = true;
						break;
					}
				}
				if ( !found ) {
					// Done here.
					vd->doneWith = sector;
					if ( debugFlag ) {
						GLOUTPUT(( "ID=%d Visitor: no path to kiosk.\n", thisComp.chit->ID() ));
					}
				}
			}
		}
	}
	if ( disconnect ) {
		parentChit->GetItem()->hp = 0;
		parentChit->SetTickNeeded();
	}
}


void AIComponent::ThinkWander( const ComponentSet& thisComp )
{
	// Wander in some sort of directed fashion.
	// - get close to friends
	// - but not too close
	// - given a choice, get close to plants.
	// - occasionally randomly wander about

	Vector2F dest = { 0, 0 };
	const GameItem* item	= parentChit->GetItem();
	int itemFlags			= item ? item->flags : 0;
	int wanderFlags			= itemFlags & GameItem::AI_WANDER_MASK;
	Vector2F pos2 = thisComp.spatial->GetPosition2D();
	Vector2I pos2i = { (int)pos2.x, (int)pos2.y };

	IRangedWeaponItem* ranged = thisComp.itemComponent->GetRangedWeapon( 0 );
	if ( ranged && ranged->GetItem()->CanReload() ) {
		ranged->GetItem()->Reload( parentChit );
	}

	// Plant eater
	if ( (itemFlags & (GameItem::AI_EAT_PLANTS | GameItem::AI_HEAL_AT_CORE)) && (item->hp < item->TotalHP()))  {
		// Are we near a plant?
		// Note that currently only support eating stage 0-1 plants.
		CChitArray plants;

		if ( itemFlags & GameItem::AI_EAT_PLANTS ) {
			PlantFilter plantFilter( -1, MAX_PASSABLE_PLANT_STAGE );
			parentChit->GetChitBag()->QuerySpatialHash( &plants, pos2, PLANT_AWARE, 0, &plantFilter );
		}
		else {
		CChitArray array;
			ItemNameFilter coreFilter( IStringConst::core );
			parentChit->GetChitBag()->QuerySpatialHash( &plants, pos2, PLANT_AWARE, 0, &coreFilter );
		}

		Vector2F plantPos =  { 0, 0 };
		float plantDist = 0;
		if ( Closest( thisComp, plants.Mem(), plants.Size(), &plantPos, &plantDist )) {
			if ( plantDist > 0.2f ) {
				dest = plantPos;
			}
			else {
				// Already where we need to be		
				if ( debugFlag ) {
					GLOUTPUT(( "ID=%d Stand\n", thisComp.chit->ID() ));
				}
				currentAction = STAND;
				return;
			}
		}
	}

	if ( dest.IsZero() ) {
		// Is there stuff around to pick up?
		if( !playerControlled && ( itemFlags & (GameItem::GOLD_PICKUP | GameItem::ITEM_PICKUP))) {

			// Which filter to use?
			GoldCrystalFilter	gold;
			LootFilter			loot;
			GoldLootFilter		both;

			IChitAccept*	accept = 0;
			if ( (itemFlags & GameItem::GOLD_PICKUP) && (itemFlags & GameItem::ITEM_PICKUP)) {
				accept = &both;
			}
			else if ( itemFlags & GameItem::GOLD_PICKUP ) {
				accept = &gold;
			}
			else {
				accept = &loot;
			}

			CChitArray chitArr;
			parentChit->GetChitBag()->QuerySpatialHash( &chitArr, pos2, GOLD_AWARE, 0, accept );

			ChitDistanceCompare compare( thisComp.spatial->GetPosition() );
			Sort<Chit*>( chitArr.Mem(), chitArr.Size(), &compare );

			for( int i=0; i<chitArr.Size(); ++i ) {
				Vector2F goldPos = chitArr[i]->GetSpatialComponent()->GetPosition2D();
				if ( map->HasStraightPath( goldPos, pos2 )) {
					// Pickup and gold use different techniques. (Because of player UI. 
					// Always want gold - not all items.)
					if ( loot.Accept( chitArr[i] )) {
						taskList.Push( Task::MoveTask( goldPos, 0 ));
						taskList.Push( Task::PickupTask( chitArr[i]->ID(), 0 ));
						return;
					}
					else {
						dest = goldPos;
					}
					break;
				}
			}
		}
	}

	// Wander....
	if ( dest.IsZero() ) {
		if ( playerControlled ) {
			currentAction = WANDER;
			return;
		}
		PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
		int r = parentChit->random.Rand(4);

		bool sectorHerd =		pmc 
							 && itemFlags & GameItem::AI_SECTOR_HERD
 							 && ( friendList.Size() >= (MAX_TRACK*3/4) || pmc->ForceCount() > FORCE_COUNT_STUCK)
							 && (thisComp.chit->random.Rand( WANDER_ODDS ) == 0);
		bool sectorWander =		pmc
							&& itemFlags & GameItem::AI_SECTOR_WANDER
							&& thisComp.item
							&& thisComp.item->HPFraction() > 0.90f
							&& (thisComp.chit->random.Rand( GREATER_WANDER_ODDS ) == 0);

		if ( sectorHerd || sectorWander ) 
		{
			if ( SectorHerd( thisComp ) )
				return;
		}
		else if ( wanderFlags == 0 || r == 0 ) {
			dest = ThinkWanderRandom( thisComp );
		}
		else if ( wanderFlags == GameItem::AI_WANDER_HERD ) {
			dest = ThinkWanderFlock( thisComp );
		}
		else if ( wanderFlags == GameItem::AI_WANDER_CIRCLE ) {
			dest = ThinkWanderCircle( thisComp );
		}
	}
	if ( !dest.IsZero() ) {
		Vector2I dest2i = { (int)dest.x, (int)dest.y };
		taskList.Push( Task::MoveTask( dest2i ));
		taskList.Push( Task::StandTask( STAND_TIME_WHEN_WANDERING ));
	}
}


void AIComponent::ThinkBattle( const ComponentSet& thisComp )
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
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
	//static  float FLOCK_MOVE_BIAS = 0.2f;
	Vector2F heading = thisComp.spatial->GetHeading2D();
	//Vector2F flockDir = heading;
	utility[OPTION_FLOCK_MOVE] = 0.001f;

	int nRangedEnemies = 0;
	for( int k=0; k<enemyList.Size(); ++k ) {
		Chit* chit = GetChitBag()->GetChit( enemyList[k] );
		if ( chit && chit->GetItemComponent() && chit->GetItemComponent()->GetRangedWeapon(0) ) {
			++nRangedEnemies;
		}
	}

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

		normalToEnemy.SafeNormalize( 1, 0 );
		float dot = DotProduct( normalToEnemy, heading );

		// Prefer targets we are pointed at.
		static const float DOT_BIAS = 0.25f;
		float q = 1.0f + dot * DOT_BIAS;
		// Prefer the current target & focused target.
		if ( enemyList[k] == targetDesc.id ) {
			q *= 2;
			if ( focus == FOCUS_TARGET ) {
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
			float longShot       = BattleMechanics::EffectiveRange( radAt1, 0.5f, 0.35f );

			// 1.5f gives spacing for bolt to start.
			// The HasRound() && !Reloading() is really important: if the gun
			// is in cooldown, don't give up on shooting and do something else!
			if (    range > 1.5f 
				 &&    ( ( pw->HasRound() && !pw->Reloading() )							// we have ammod
				    || ( nRangedEnemies == 0 && range > 2.0f && range < longShot ) ) )	// we need to reload
			{
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
			float u = MELEE_RANGE / range;
			u *= q;
			// Utility of the actual charge vs. moving closer. This
			// seems to work with an if case.
			if ( range > MELEE_RANGE * 3.0f ) {
				if ( u > utility[OPTION_MOVE_TO_RANGE] ) {
					utility[OPTION_MOVE_TO_RANGE] = u;
					//moveToRange = pos2 + normalToEnemy * (range - meleeRange*2.0f);
					moveToRange = enemyPos2;
					moveToTime = (range - MELEE_RANGE*2.0f) / pmc->Speed();
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

	// Translate to action system:
	switch ( index ) {
		case OPTION_FLOCK_MOVE:
		{
			//Vector2F dest = pos2 + flockDir;
			//this->Move( dest, false );
			pmc->Stop();
		}
		break;

		case OPTION_MOVE_TO_RANGE:
		{
			targetDesc.Set( target[OPTION_MOVE_TO_RANGE]->ID() );
			GLASSERT( targetDesc.HasTarget() );
			this->Move( moveToRange, false );
		}
		break;

		case OPTION_MELEE:
		{
			currentAction = MELEE;
			targetDesc.Set( target[OPTION_MELEE]->ID() );
			GLASSERT( targetDesc.HasTarget() );
		}
		break;
		
		case OPTION_SHOOT:
		{
			pmc->Stop();
			currentAction = SHOOT;
			targetDesc.Set( target[OPTION_SHOOT]->ID() );
			GLASSERT( targetDesc.HasTarget() );
		}
		break;

		default:
			GLASSERT( 0 );
	};

	if ( debugFlag ) {
		static const char* optionName[NUM_OPTIONS] = { "flock", "mtrange", "melee", "shoot" };
		GLOUTPUT(( "ID=%d Think: flock=%.2f mtrange=%.2f melee=%.2f shoot=%.2f -> %s\n",
				   thisComp.chit->ID(), utility[OPTION_FLOCK_MOVE], utility[OPTION_MOVE_TO_RANGE], utility[OPTION_MELEE], utility[OPTION_SHOOT],
				   optionName[index] ));
	}
}


void AIComponent::FlushTaskList( const ComponentSet& thisComp )
{
	if ( taskList.Empty() ) return;
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( !pmc ) {
		taskList.Clear();
		return;
	}

	Task* task = &taskList[0];
	Vector2I pos2i = thisComp.spatial->GetPosition2DI();
	Vector2F taskPos2 = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
	Vector3F taskPos3 = { taskPos2.x, 0, taskPos2.y };
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	WorkQueue* workQueue = 0;
	const WorkQueue::QueueItem* queueItem = 0;

	// If this is a task associated with a work item, make
	// sure that work item still exists.
	if ( task->taskID ) {
		CoreScript* coreScript = GetChitBag()->ToLumos()->GetCore( sector );
		if ( coreScript ) {
			workQueue = coreScript->GetWorkQueue();
			if ( workQueue ) {
				queueItem = workQueue->GetJobByTaskID( task->taskID );
			}
		}
		if ( !queueItem ) {
			// This task it attached to a work item that expired.
			taskList.Clear();
			return;
		}
	}

	switch ( task->action ) {
	case MOVE:
		if ( pmc->Stopped() ) {
			if ( pos2i == task->pos2i ) {
				// arrived!
				taskList.PopFront();
			}
			else {
				Vector2F dest = { (float)task->pos2i.x + 0.5f, (float)task->pos2i.y + 0.5f };
				Move( dest, false );
			}
		}
		break;

	case MELEE:
	case SHOOT:
	case WANDER:
		GLASSERT( 0 );
		taskList.PopFront();
		break;

	case STAND:
		if ( pmc->Stopped() ) {
			currentAction = STAND;
			// DoStand will decrement timer.
			if ( task->timer <= 0 ) {
				taskList.PopFront();
			}
		}
		break;

	case TASK_REMOVE:
		{
			//GLASSERT( workQueue && queueItem );
			//if ( workQueue->TaskCanComplete( *queueItem )) {
			if ( workQueue->TaskCanComplete( map, GetLumosChitBag(), task->pos2i, false, 1 )) {
				const WorldGrid& wg = map->GetWorldGrid( task->pos2i.x, task->pos2i.y );
				if ( wg.RockHeight() ) {
					DamageDesc dd( 10000, 0 );	// FIXME need constant
					Vector3I voxel = { task->pos2i.x, 0, task->pos2i.y };
					map->VoxelHit( voxel, dd );
				}
				else {
					Chit* found = GetLumosChitBag()->QueryRemovable( task->pos2i );
					if ( found ) {
						DamageDesc dd( 10000, 0 );
						ChitDamageInfo info( dd );
						info.originID = 0;
						info.awardXP  = false;
						info.isMelee  = true;
						info.isExplosion = false;
						info.originOfImpact = thisComp.spatial->GetPosition();
						found->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &info ), 0 );
					}
				}
				if ( wg.Pave() ) {
					map->SetPave( task->pos2i.x, task->pos2i.y, 0 );
				}
				taskList.PopFront();
			}
			else {
				taskList.Clear();
			}
		}
		break;

	case TASK_BUILD:
		{
			// IsPassable (isLand, noRocks)
			// No plants
			// No buildings
			GLASSERT( workQueue && queueItem );
			if ( workQueue->TaskCanComplete( *queueItem )) {
				if ( task->structure.empty() ) {
					map->SetRock( task->pos2i.x, task->pos2i.y, 1, false, WorldGrid::ICE );
				}
				else {
					if ( task->structure == "pave" ) {
						map->SetPave( task->pos2i.x, task->pos2i.y, thisComp.item->primaryTeam%3+1 );
					}
					else {
						GetChitBag()->ToLumos()->NewBuilding( task->pos2i, task->structure.c_str(), thisComp.item->primaryTeam );
					}
				}
				taskList.PopFront();
			}
			else {
				// Plan has gone bust:
				taskList.Clear();
			}
		}
		break;

	case TASK_PICKUP:
		{
			int chitID = task->data;
			Chit* itemChit = GetChitBag()->GetChit( chitID );
			if (    itemChit 
				 && itemChit->GetSpatialComponent()
				 && itemChit->GetSpatialComponent()->GetPosition2DI() == parentChit->GetSpatialComponent()->GetPosition2DI()
				 && itemChit->GetItemComponent()->NumItems() == 1 )	// doesn't have sub-items / intrinsic
			{
				ItemComponent* ic = itemChit->GetItemComponent();
				itemChit->Remove( ic );
				parentChit->GetItemComponent()->AddToInventory( ic );
				GetChitBag()->DeleteChit( itemChit );
			}
			taskList.PopFront();
		}
		break;

	}
}


void AIComponent::WorkQueueToTask(  const ComponentSet& thisComp )
{
	// Is there work to do?		
	Vector2I sector = thisComp.spatial->GetSector();
	CoreScript* coreScript = GetChitBag()->ToLumos()->GetCore( sector );
	if ( coreScript ) {
		WorkQueue* workQueue = coreScript->GetWorkQueue();

		// Get the current job, or find a new one.
		const WorkQueue::QueueItem* item = workQueue->GetJob( parentChit->ID() );
		if ( !item ) {
			item = workQueue->Find( thisComp.spatial->GetPosition2DI() );
			if ( item ) {
				workQueue->Assign( parentChit->ID(), item );
			}
		}
		if ( item ) {
			switch ( item->action )
			{
			case WorkQueue::CLEAR:
				{
					Vector2F dest = { (float)item->pos.x + 0.5f, (float)item->pos.y + 0.5f };
					Vector2F end;
					float cost = 0;
					if ( map->CalcPathBeside( thisComp.spatial->GetPosition2D(), dest, &end, &cost )) {
						
						taskList.Push( Task::MoveTask( end, item->taskID ));
						taskList.Push( Task::StandTask( 1000, item->taskID ));
						taskList.Push( Task::RemoveTask( item->pos, item->taskID ));
					}
				}
				break;

			case WorkQueue::BUILD:
				{
					taskList.Push( Task::MoveTask( item->pos, item->taskID ));
					taskList.Push( Task::StandTask( 1000, item->taskID ));
					taskList.Push( Task::BuildTask( item->pos, item->structure, item->taskID ));
				}
				break;

				/*
			case WorkQueue::PAVE:
				{
					Vector2F dest = { (float)item->pos.x + 0.5f, (float)item->pos.y + 0.5f };
					if ( WorkQueue::TaskCanComplete( map, GetChitBag()->ToLumos(), item->pos, false, 1 )) {
						Vector2F end;
						float cost = 0;
						if ( map->CalcPathBeside( thisComp.spatial->GetPosition2D(), dest, &end, &cost )) {
						
							taskList.Push( Task::MoveTask( end, item->taskID ));
							taskList.Push( Task::StandTask( 1000, item->taskID ));
							taskList.Push( Task::RemoveTask( item->pos, item->taskID ));
						}
					}
					taskList.Push( Task::MoveTask( dest, item->taskID ));
					taskList.Push( Task::StandTask( 1000, item->taskID ));
					taskList.Push( Task::PaveTask( item->pos, item->pave, item->taskID ));
				}
				break;
				*/

			default:
				GLASSERT( 0 );
				break;
			}
		}
	}
}



int AIComponent::DoTick( U32 deltaTime, U32 timeSince )
{
	//GRINLIZ_PERFTRACK;
	PROFILE_FUNC();

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
	friendEnemyAge += timeSince;
	int oldAction = currentAction;

	ChitBag* chitBag = this->GetChitBag();

	// Focuesd move check
	if ( focus == FOCUS_MOVE ) {
		return GetThinkTime();
	}

	// If focused, make sure we have a target.
	if ( targetDesc.id && !chitBag->GetChit( targetDesc.id )) {
		targetDesc.Clear();
		currentAction = 0;
	}
	if ( !targetDesc.id ) {
		focus = FOCUS_NONE;
	}

	if ( friendEnemyAge > 750 ) {
		GetFriendEnemyLists();
		friendEnemyAge = parentChit->ID() & 63;	// a little randomness
	}

	// High level mode switch, in/out of battle?
	if ( aiMode != BATTLE_MODE && enemyList.Size() ) {
		aiMode = BATTLE_MODE;
		currentAction = 0;
		taskList.Clear();
		if ( debugFlag ) {
			GLOUTPUT(( "ID=%d Mode to Battle\n", thisComp.chit->ID() ));
		}
	}
	else if ( aiMode == BATTLE_MODE && targetDesc.id == 0 && enemyList.Empty() ) {
		aiMode = NORMAL_MODE;
		currentAction = 0;
		if ( debugFlag ) {
			GLOUTPUT(( "ID=%d Mode to Normal\n", thisComp.chit->ID() ));
		}
	}

	// Is there work to do?
	if (    aiMode == NORMAL_MODE 
		 && (thisComp.item->flags & GameItem::AI_DOES_WORK)
		 && taskList.Empty() ) 
	{
		WorkQueueToTask( thisComp );
	}

	if ( aiMode == NORMAL_MODE && !taskList.Empty() ) {
		FlushTaskList( thisComp );
		if ( taskList.Empty() ) {
			rethink = 0;
		}
	}
	else if ( !currentAction || (rethink > 1000 ) ) {
		Think( thisComp );
		rethink = 0;
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	switch( currentAction ) {

		case MOVE:		
			DoMove( thisComp );
			rethink += deltaTime;
			break;
		case MELEE:		
			DoMelee( thisComp );	
			break;
		case SHOOT:		
			DoShoot( thisComp );
			break;
		case STAND:
			if ( !DoStand( thisComp, timeSince ) ) {
				rethink += deltaTime;
			}
			break;

		case NO_ACTION:
			break;

		case WANDER:
			{
				PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
				if ( pmc && !pmc->Stopped() && !pmc->ForceCountHigh() ) {
					// okay
				}
				else {
					// not actually wandering
					currentAction = STAND;
				}
			}
			break;

		default:
			GLASSERT( 0 );
			currentAction = 0;
			break;
	}
	if ( debugFlag && (currentAction != oldAction) ) {
		GLOUTPUT(( "ID=%d mode=%s action=%s\n", thisComp.chit->ID(), MODE_NAMES[aiMode], ACTION_NAMES[currentAction] ));
	}
	return (currentAction != NO_ACTION) ? GetThinkTime() : 0;
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] %s %s ", MODE_NAMES[aiMode], ACTION_NAMES[currentAction] );
}


void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	/* Remember this is a *synchronous* function.
	   Not safe to reach back and change other components.
	*/
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );

	switch ( msg.ID() ) {
	case ChitMsg::PATHMOVE_DESTINATION_REACHED:
		if ( currentAction != WANDER ) {
			focus = 0;
			currentAction = NO_ACTION;
			parentChit->SetTickNeeded();
		}
		// FIXME: when should an AI bind to core? This just does
		// it at the end of a move, irrespective of the current
		// action or mode.
		if ( chit->GetItem() && ( chit->GetItem()->flags & GameItem::AI_BINDS_TO_CORE )) {

			Vector2I mapPos = thisComp.spatial->GetPosition2DI();
			Vector2I sector = { mapPos.x/SECTOR_SIZE, mapPos.y/SECTOR_SIZE };
			bool standingOnCore = map->GetWorldGrid( mapPos.x, mapPos.y ).IsCore();

			if ( standingOnCore ) {
				CoreScript* boundCore = GetChitBag()->ToLumos()->IsBoundToCore( parentChit, false );
				CoreScript* thisCore  = GetChitBag()->ToLumos()->GetCore( sector );

				// If the MOB is bound to a core in a different sector it can't bind here. BUT,
				// it can re-bind to this core if needed.
				if ( boundCore && (boundCore != thisCore )) {
					// Do nothing. Bound somewhere else.
				}
				else {
					// Bind or re-bind
					GLASSERT( thisCore );	// we should be standing on one. 
					thisCore->AttachToCore( parentChit );
				}
			}
		}
		break;

	case ChitMsg::PATHMOVE_DESTINATION_BLOCKED:
		focus = 0;
		currentAction = NO_ACTION;
		parentChit->SetTickNeeded();

		{
			WorkQueue* workQueue = GetWorkQueue();
			if ( workQueue && workQueue->GetJob( parentChit->ID() )) {
				workQueue->ReleaseJob( parentChit->ID() );
				// total re-think.
				aiMode = NORMAL_MODE;
			}
		}
		taskList.Clear();
		{
			PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
			if ( pmc ) {
				pmc->Clear();	// make sure to clear the path and the queued path
			}
		}
		break;

	case ChitMsg::CHIT_SECTOR_HERD:
		{
			// Read our destination port information:
			const SectorPort* sectorPort = (const SectorPort*) msg.Ptr();
			this->Move( *sectorPort, false );
		}
		break;

	case ChitMsg::WORKQUEUE_UPDATE:
		if ( aiMode == NORMAL_MODE && currentAction == WANDER ) {
			currentAction = NO_ACTION;
			parentChit->SetTickNeeded();
		}
		break;

	default:
		super::OnChitMsg( chit, msg );
		break;
	}
}


