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

#include "../script/battlemechanics.h"
#include "../script/plantscript.h"
#include "../script/worldgen.h"
#include "../script/corescript.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

#include "../grinliz/glrectangle.h"
#include "../grinliz/glperformance.h"
#include <climits>

using namespace grinliz;

static const float NORMAL_AWARENESS		= 10.0f;
static const float LOOSE_AWARENESS		= 16.0f;
static const float SHOOT_ANGLE			= 10.0f;	// variation from heading that we can shoot
static const float SHOOT_ANGLE_DOT		=  0.985f;	// same number, as dot product.
static const float WANDER_RADIUS		=  5.0f;
static const float EAT_HP_PER_SEC		=  2.0f;
static const float EAT_HP_HEAL_MULT		=  5.0f;	// eating really tears up plants. heal the critter more than damage the plant.
static const int   WANDER_ODDS			= 50;		// as in 1 in WANDER_ODDS
static const float PLANT_AWARE			=  3.0f;
static const float GOLD_AWARE			=  3.5f;
static const int   FORCE_COUNT_STUCK	=  8;


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

	Vector3F origin, dest;
	IRangedWeaponItem* weapon = thisComp.itemComponent->GetRangedWeapon( &origin );
	GLASSERT( weapon );

	ComponentSet target( t, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	if ( !target.okay ) {
		return false;
	}
	target.render->GetMetaData( IStringConst::ktarget, &dest );

	Vector3F dir = dest - origin;
	float length = dir.Length() + 0.01f;	// a little extra just in case
	CArray<const Model*, EL_MAX_METADATA+2> ignore, targetModels;
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
	CArray<const Model*, EL_MAX_METADATA+2> ignore;
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



static bool FEFilter( Chit* c ) {
	// Tricky stuff. This will return F/E units:
	if( c->GetAIComponent() )
		return true;
	// Dummy targets, enemy structures.
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
	GetChitBag()->QuerySpatialHash( &chitArr, zone, parentChit, FEFilter );
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
	if ( !pmc || pmc->ForceCountHigh() ) {
		currentAction = NO_ACTION;
		return;
	}

	// Generally speaking, moving is done by the PathMoveComponent. When
	// not in battle, this is essentially "do nothing." If in battle mode,
	// we look for opportunity fire and such.
	if ( aiMode != BATTLE_MODE ) {
		// Check for motion done, stuck, etc.
		if ( !pmc->IsMoving() || pmc->ForceCountHigh() ) {
			currentAction = NO_ACTION;
			return;
		}
		// Reloading is always a good background task.
		IRangedWeaponItem* rangedWeapon = thisComp.itemComponent->GetRangedWeapon( 0 );
		if ( rangedWeapon && rangedWeapon->GetItem()->CanReload() ) {
			rangedWeapon->GetItem()->Reload();
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
					ranged->Reload();
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
	else {
		GLASSERT( !targetDesc.mapPos.IsZero() );
		leading.Set( (float)targetDesc.mapPos.x + 0.5f, 0.5f, (float)targetDesc.mapPos.y + 0.5f );
	}
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
			item->Reload();
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
		 && (itemFlags & GameItem::AI_EAT_PLANTS) 
		 && (item->hp < totalHP ))  
	{
		// Are we on a plant?
		Vector2F pos = thisComp.spatial->GetPosition2D();
		// Note that currently only support eating stage 0-1 plants.
		CChitArray plants;
		parentChit->GetChitBag()->QuerySpatialHash( &plants, pos, 0.4f, 0, PlantScript::PassablePlantFilter );
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
	else if (    visitorIndex >= 0
		      && !thisComp.move->IsMoving() )
	{
		// Visitors at a kiosk.
		// Are we on a kiosk?
		Vector2I pos2i = thisComp.spatial->GetPosition2DI();
		Chit* chit = this->GetChitBag()->ToLumos()->QueryBuilding( pos2i, true );
		if ( chit && chit->GetItem()->name == "kiosk" ) {
			VisitorData* vd = &Visitors::Instance()->visitorData[visitorIndex];
			vd->kioskTime += since;
			if ( vd->kioskTime > VisitorData::KIOSK_TIME ) {
				Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
				vd->sectorVisited.Push( sector );
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
	case BUILD_MODE:		ThinkBuild( thisComp );		break;
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


void AIComponent::Move( const grinliz::Vector2F& dest, bool focused )
{
	PathMoveComponent* pmc    = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( pmc ) {
		pmc->QueueDest( dest, -1 );
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
	GLOUTPUT(( "Melt something at %d,%d\n", rock.x, rock.y ));
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay ) 
		return false;

	const WorldGrid& wg = map->GetWorldGrid( rock.x, rock.y );
	if ( wg.RockHeight() == 0 )
		return false;

	aiMode = ROCKBREAK_MODE;
	currentAction = NO_ACTION;
	targetDesc.Set( rock );
	parentChit->SetTickNeeded();
	return true;
}


bool AIComponent::Build( const grinliz::Vector2I& pos, IString structure )
{
	GLOUTPUT(( "Ice at %d,%d\n", pos.x, pos.y ));
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay ) 
		return false;

	if ( map->IsPassable( pos.x, pos.y )) {
		aiMode = BUILD_MODE;
		currentAction = NO_ACTION;
		targetDesc.Set( pos, structure );
		parentChit->SetTickNeeded();
		return true;
	}
	return false;
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


void AIComponent::ThinkBuild( const ComponentSet& thisComp )
{
	if ( !thisComp.okay ) {
		currentAction = NO_ACTION;
		return;
	}

	// Make sure:
	//	- there is an item in the WorkQueue
	//	- the destination is availabe
	//	- we are going (or at) the destination.
	// FIXME: not accounting for getting stuck trying to get to destination.

	Vector2F pos2			= thisComp.spatial->GetPosition2D();
	Vector2I pos2i			= { (int)pos2.x, (int)pos2.y };
	Vector2I sector			= { pos2i.x / SECTOR_SIZE, pos2i.y / SECTOR_SIZE };

	WorkQueue* workQueue	= GetWorkQueue();
	if ( !workQueue ) { currentAction = NO_ACTION; return; }

	const WorkQueue::QueueItem* item = workQueue->GetJob( parentChit->ID() );
	if ( !item ) { 
		aiMode = NORMAL_MODE;
		currentAction = NO_ACTION; 
		return; 
	}
	if ( item->action < WorkQueue::BUILD_START || item->action >= WorkQueue::BUILD_END ) {
		// not sure how this happened.
		workQueue->ReleaseJob( parentChit->ID() );
		aiMode = NORMAL_MODE;
		currentAction = NO_ACTION; 
		return; 
	}

	// Is the item where we are going?
	if ( item->pos == pos2i ) {
		// WE ARRIVE! Build. No delay. Maybe in the future.
		//GetChitBag()->ToLumos()->NewBuilding( item->building );
		if ( map->IsPassable( pos2i.x, pos2i.y )) {
			if ( item->action == WorkQueue::BUILD_ICE ) {
				map->SetRock( pos2i.x, pos2i.y, 1, false, WorldGrid::ICE );
			}
			else if ( item->action == WorkQueue::BUILD_STRUCTURE ) {
				GLASSERT( !item->structure.empty() );
				GetChitBag()->ToLumos()->NewBuilding( pos2i, item->structure.c_str(), thisComp.item->primaryTeam );
			}
			aiMode = NORMAL_MODE;
			currentAction = NO_ACTION;
		}
		return;
	}

	// Can we go there?
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( !pmc ) { currentAction = NO_ACTION; return; }

	Vector2F dest = pmc->DestPos();
	Vector2I desti = { (int)dest.x, (int)dest.y };
	if ( desti != item->pos || pmc->ForceCountHigh() ) {
		dest.Set( (float)item->pos.x + 0.5f, (float)item->pos.y+0.5f );
		pmc->QueueDest( dest );
		currentAction = MOVE;
	}
}

	
void AIComponent::ThinkRockBreak( const ComponentSet& thisComp )
{
	//GRINLIZ_PERFTRACK;
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	const WorldGrid& wg = map->GetWorldGrid( targetDesc.mapPos.x, targetDesc.mapPos.y );

	if ( wg.RockHeight() == 0 || !pmc ) {
		currentAction	= NO_ACTION;
		aiMode			= NORMAL_MODE;
		return;
	}

	const Vector3F& pos = thisComp.spatial->GetPosition();
	Vector2F pos2		= { pos.x, pos.z };
	Vector3F rockTarget	= targetDesc.MapTarget();
	Vector2I rock2i		= { (int)rockTarget.x, (int)rockTarget.z };
	
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
		currentAction = SHOOT;
		return;
	}
	else if ( !lineOfSight || meleeWeapon ) {
		// Move to target
		Vector2F dest = { (float)targetDesc.mapPos.x + 0.5f, (float)targetDesc.mapPos.y + 0.5f };
		Vector2F end;
		float cost = 0;
		if ( map->CalcPathBeside( thisComp.spatial->GetPosition2D(), dest, &end, &cost )) {
			currentAction = MOVE;
			pmc->QueueDest( end );
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
	Vector2F dest = GetWanderOrigin( thisComp );
	dest.x += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
	dest.y += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
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
	pos.Push( GetWanderOrigin( thisComp ) );	// the origin is a friend.

	// And plants are friends.
	Rectangle2F r;
	r.min = r.max = origin;
	r.Outset( PLANT_AWARE );

	CChitArray plants;
	parentChit->GetChitBag()->QuerySpatialHash( &plants, r, 0, PlantScript::PassablePlantFilter );
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
	if ( pmc->IsMoving() && !pmc->ForceCountHigh() ) return;	// everything is okay. move along.

	bool disconnect = false;

	Vector2I pos2i = thisComp.spatial->GetPosition2DI();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	VisitorData* vd = &Visitors::Instance()->visitorData[visitorIndex];	// FIXME ugly
	Chit* kiosk = GetChitBag()->ToLumos()->QueryBuilding( pos2i, true );
	if ( kiosk && kiosk->GetItem()->name == "kiosk" ) {
		// all good
	}
	else {
		kiosk = 0;
	}

	if ( !vd->sectorVisited.HasCap() ) {
		disconnect = true;
	}
	else {
		// Move on,
		// Stand, or
		// Move to a kiosk
		if ( vd->sectorVisited.Find( sector ) >= 0 ) {
			// Head out!
			// FIXME: smart destination.
			SectorPort sp = map->RandomPort( &parentChit->random );
			bool okay = this->Move( sp, true );
			if ( !okay ) disconnect = true;
		}
		else if ( !pmc->IsMoving() && kiosk ) {
			currentAction = STAND;
		}
		else {
			// Find a kiosk.
			CChitArray arr;
			Rectangle2I inneri = map->GetSector( sector ).InnerBounds();
			Rectangle2F inner;
			inner.Set( (float)inneri.min.x, (float)inneri.min.y, (float)(inneri.max.x+1), (float)(inneri.max.y+1) );

			parentChit->GetChitBag()->QuerySpatialHash( &arr, inner, 0, LumosChitBag::KioskFilter );
			if ( arr.Empty() ) {
				// Done here.
				vd->sectorVisited.Push( sector );
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
					vd->sectorVisited.Push( sector );
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
	int actionToTake		= WANDER;
	Vector2F pos2 = thisComp.spatial->GetPosition2D();
	Vector2I pos2i = { (int)pos2.x, (int)pos2.y };

	IRangedWeaponItem* ranged = thisComp.itemComponent->GetRangedWeapon( 0 );
	if ( ranged && ranged->GetItem()->CanReload() ) {
		ranged->GetItem()->Reload();
	}

	// Plant eater
	if ( (itemFlags & GameItem::AI_EAT_PLANTS) && (item->hp < item->TotalHP()))  {
		// Are we near a plant?
		// Note that currently only support eating stage 0-1 plants.
		CChitArray plants;
		parentChit->GetChitBag()->QuerySpatialHash( &plants, pos2, PLANT_AWARE, 0, PlantScript::PassablePlantFilter );

		Vector2F plantPos =  { 0, 0 };
		float plantDist = 0;
		if ( Closest( thisComp, plants.Mem(), plants.Size(), &plantPos, &plantDist )) {
			if ( plantDist > 0.2f ) {
				actionToTake = MOVE;
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
		if( thisComp.itemComponent->Pickup() == ItemComponent::GOLD_PICKUP ) {
			CChitArray gold;
			parentChit->GetChitBag()->QuerySpatialHash( &gold, pos2, GOLD_AWARE, 0, LumosChitBag::GoldFilter );

			Vector2F goldPos;
			if ( Closest( thisComp, gold.Mem(), gold.Size(), &goldPos, 0 ) ) {
				actionToTake = MOVE;
				dest = goldPos;
			}
		}
	}
	// Wander....
	if ( dest.IsZero() ) {
		if ( wanderFlags == 0 ) {
			currentAction = WANDER;
			return;
		}
		PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );

		if (    pmc 
			 && pmc->ForceCount() > FORCE_COUNT_STUCK 
			 && itemFlags & GameItem::AI_SECTOR_HERD
 			 && friendList.Size() >= (MAX_TRACK*3/4)
			 && (thisComp.chit->random.Rand( WANDER_ODDS ) == 0) ) 
		{
			if ( SectorHerd( thisComp ) )
				return;
		}
		else if ( parentChit->random.Rand(3) == 0 ) {
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
		PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
		if ( pmc ) {
			pmc->QueueDest( dest );
		}
		currentAction = actionToTake;
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
	static  float FLOCK_MOVE_BIAS = 0.2f;
	Vector2F heading = thisComp.spatial->GetHeading2D();
	Vector2F flockDir = heading;
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
			currentAction = MOVE;
			Vector2F dest = pos2 + flockDir;
			pmc->QueueDest( dest );
		}
		break;

		case OPTION_MOVE_TO_RANGE:
		{
			currentAction = MOVE;
			targetDesc.Set( target[OPTION_MOVE_TO_RANGE]->ID() );
			pmc->QueueDest( moveToRange );
		}
		break;

		case OPTION_MELEE:
		{
			currentAction = MELEE;
			targetDesc.Set( target[OPTION_MELEE]->ID() );
		}
		break;
		
		case OPTION_SHOOT:
		{
			pmc->Stop();
			currentAction = SHOOT;
			targetDesc.Set( target[OPTION_SHOOT]->ID() );
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


int AIComponent::DoTick( U32 deltaTime, U32 timeSince )
{
	GRINLIZ_PERFTRACK;

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
	if ( aiMode == NORMAL_MODE && (thisComp.item->flags & GameItem::AI_DOES_WORK) ) {
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
				case WorkQueue::CLEAR_GRID:
					RockBreak( item->pos );
					break;

				case WorkQueue::BUILD_ICE:
					Build( item->pos, IString() );
					break;

				case WorkQueue::BUILD_STRUCTURE:
					Build( item->pos, item->structure );
					break;

				default:
					GLASSERT( 0 );
					break;
				}
			}
		}
	}

	if ( !currentAction || (rethink > 1000 ) ) {
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
				if ( pmc && pmc->IsMoving() && !pmc->ForceCountHigh() ) {
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
		static const char* modes[NUM_MODES]     = { "normal", "rockbreak", "battle" };
		static const char* actions[NUM_ACTIONS] = { "none", "move", "melee", "shoot", "wander", "stand" };
		GLOUTPUT(( "ID=%d mode=%s action=%s\n", thisComp.chit->ID(), modes[aiMode], actions[currentAction] ));
	}
	return (currentAction != NO_ACTION) ? GetThinkTime() : 0;
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
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
			Vector2F center = thisComp.spatial->GetPosition2D();
			center.x = floorf( center.x ) + 0.5f;
			center.y = floorf( center.y ) + 0.5f;
			CChitArray arr;
			GetChitBag()->QuerySpatialHash( &arr, center, 0.1f, parentChit, LumosChitBag::CoreFilter );
			if ( arr.Size() ) {
				ScriptComponent* sc = arr[0]->GetScriptComponent();
				GLASSERT( sc && sc->Script() );
				CoreScript* coreScript = sc->Script()->ToCoreScript();
				GLASSERT( coreScript );
				coreScript->AttachToCore( parentChit );
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
			}
		}
		break;

	case ChitMsg::CHIT_SECTOR_HERD:
		{
			// Read our destination port information:
			const SectorPort* sectorPort = (const SectorPort*) msg.Ptr();
			this->Move( *sectorPort, true );
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


