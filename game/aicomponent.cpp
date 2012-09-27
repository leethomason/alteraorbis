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

#include "../script/battlemechanics.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/inventorycomponent.h"

#include "../grinliz/glrectangle.h"
#include <climits>

using namespace grinliz;

static const float COMBAT_INFO_RANGE	= 10.0f;	// range around to scan for friendlies/enemies
static const float LONGEST_WEAPON_RANGE = 20.0f;

static const float PRIMARY_UTILITY		= 10.0f;
static const float SECONDARY_UTILITY	= 3.0f;
static const float TERTIARY_UTILITY		= 1.0f;

static const float LOW_UTILITY			= 0.2f;
static const float MED_UTILITY			= 0.5f;
static const float HIGH_UTILITY			= 0.8f;

//#define AI_OUTPUT

AIComponent::AIComponent( Engine* _engine, WorldMap* _map )
{
	engine = _engine;
	map = _map;
	currentAction = 0;
}


AIComponent::~AIComponent()
{
}


int AIComponent::GetTeamStatus( Chit* other )
{
	// FIXME: placeholder friend/enemy logic
	ItemComponent* thisItem  = GET_COMPONENT( parentChit, ItemComponent );
	ItemComponent* otherItem = GET_COMPONENT( other, ItemComponent );
	if ( thisItem && otherItem ) {
		if ( thisItem->item.primaryTeam != otherItem->item.primaryTeam ) {
			return ENEMY;
		}
	}
	return FRIENDLY;
}


bool AIComponent::LineOfSight( Chit* src, Chit* t )
{
	ComponentSet thisComp( src, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	ComponentSet target( t, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );

	if ( !thisComp.okay || !target.okay )
		return false;

	Vector3F origin, dest;
	thisComp.render->GetMetaData( "trigger", &origin );		// FIXME: not necessarily trigger; get correct hardpoint. 
	target.render->GetMetaData( "target", &dest );
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


void AIComponent::UpdateCombatInfo( const Rectangle2F* _zone )
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return;
	Vector2F center = sc->GetPosition2D();

	Rectangle2F zone;
	if ( !_zone ) {
		// Generate the default zone.
		zone.min = center; zone.max = center;
		zone.Outset( COMBAT_INFO_RANGE );
	}
	else {
		// Use the passed in info, add to the existing.
		zone = *_zone;
	}

	// Sort in by as-the-crow-flies range. Not correct, but don't want to deal with arbitrarily long query.
	GetChitBag()->QuerySpatialHash( &chitArr, zone, parentChit, GameItem::CHARACTER );

	// This is surprisingly subtle. On the one hand, if we don't find anything, we
	// don't want to clear existing targets. (Guys just stand around.) On the other
	// hand, we don't want old info in there.
	if ( !chitArr.Empty() ) {
		bool friendClear = false;
		bool enemyClear = false;

		for( int i=0; i<chitArr.Size(); ++i ) {
			Chit* chit = chitArr[i];

			int teamStatus = GetTeamStatus( chit );

			if ( teamStatus == FRIENDLY ) {
				if (!friendClear ) {
					friendList.Clear();
					friendClear = true;
				}
				if ( friendList.HasCap() )
					friendList.Push( chit->ID() );
			}
			else if ( teamStatus == ENEMY ) {
				if (!enemyClear ) {
					enemyList.Clear();
					enemyClear = true;
				}
				if ( enemyList.HasCap() )
					enemyList.Push( chit->ID() );
			}
		}
	}
	Think();
}


void AIComponent::DoShoot()
{
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   Chit::INVENTORY_BIT |		// need to be carrying a melee weapon
									   ComponentSet::IS_ALIVE );
	if ( !thisComp.okay )
		return;

	// FIXME: rotation should be continuous, not just when not pointed the correct way
	bool pointed = false;
	Chit* targetChit = 0;

	while ( !enemyList.Empty() ) {
		ComponentSet target( GetChit( enemyList[0] ), Chit::RENDER_BIT |
													  Chit::SPATIAL_BIT |
													  ComponentSet::IS_ALIVE );
		if ( !target.okay ) {
			enemyList.SwapRemove( 0 );
			continue;
		}
		targetChit = target.chit;
	
		// Rotate to target.
		// FIXME: normal to target should be based on 'trigger'
		Vector2F heading = thisComp.spatial->GetHeading2D();
		float headingAngle = RotationXZDegrees( heading.x, heading.y );

		Vector2F normalToTarget = target.spatial->GetPosition2D() - thisComp.spatial->GetPosition2D();
		float distanceToTarget = normalToTarget.Length();
		normalToTarget.Normalize();
		float angleToTarget = RotationXZDegrees( normalToTarget.x, normalToTarget.y );

		static const float ANGLE = 10.0f;
		if ( fabsf( headingAngle - angleToTarget ) < ANGLE ) {
			pointed = true;
		}
		else {
			PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
			if ( pmc ) {
				pmc->QueueDest( thisComp.spatial->GetPosition2D(), angleToTarget );
			}
		}
		break;
	}

	if ( pointed ) {
		CArray< InventoryComponent::RangedInfo, NUM_HARDPOINTS > weapons;
		thisComp.inventory->GetRangedWeapons( &weapons );

		// FIXME: choose best weapon, not just first one that works.
		for( int i=0; i<weapons.Size(); ++i ) {
			GameItem* item = weapons[i].weapon->GetItem();
			if ( item->Ready() ) {
				if ( item->HasRound() ) {
					battleMechanics.Shoot( GetChitBag(), parentChit, targetChit, weapons[i].weapon, weapons[i].trigger );
				}
				else {
					item->Reload();
				}
				break;
			}
		}
	}
}


void AIComponent::DoMelee()
{
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   Chit::INVENTORY_BIT |		// need to be carrying a melee weapon
									   ComponentSet::IS_ALIVE |
									   ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay )
		return;
	
	IMeleeWeaponItem* weapon = thisComp.inventory->GetMeleeWeapon();
	if ( !weapon ) 
		return;
	GameItem* item = weapon->GetItem();

	// Are we close enough to hit? Then swing. Else move to target.
	Chit* targetChit = 0;
	bool repath = false;

	while ( !enemyList.Empty() ) {
		targetChit = parentChit->GetChitBag()->GetChit( enemyList[0] );
		if ( targetChit )
			break;
		enemyList.SwapRemove(0);
		repath = true;
	}
	ComponentSet target( targetChit, Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );
	if ( !target.okay ) {
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		if ( pmc ) {
			pmc->QueueDest( parentChit );	// stop moving.
		}
		currentAction = NO_ACTION;
		return;
	}

	if ( battleMechanics.InMeleeZone( engine, parentChit, targetChit ) ) {
		if ( item->Ready() ) {
			GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
			parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
		}
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
			if ( repath || delta > distanceToTarget * 0.25f ) {
				pmc->QueueDest( targetChit );
			}
		}
	}
}


void AIComponent::OnChitEvent( const ChitEvent& event )
{
	if ( event.ToAwareness() ) {
		ItemComponent* itemComp = GET_COMPONENT( parentChit, ItemComponent );
		if ( itemComp ) {
			const AwarenessChitEvent* aware = event.ToAwareness();
			if ( aware->Team() == itemComp->item.primaryTeam ) 
			{
				UpdateCombatInfo( &aware->Bounds() );
			}
		}
	}
}


void AIComponent::Think()
{
	// This may get called when there is an action, and update.
	// Or there may be no action.

	ComponentSet thisComp( parentChit,	Chit::SPATIAL_BIT | 
										Chit::ITEM_BIT | 
										ComponentSet::IS_ALIVE |
										ComponentSet::NOT_IN_IMPACT );
	if ( !thisComp.okay )
		return;

	const Vector3F& pos = thisComp.spatial->GetPosition();

	CArray< InventoryComponent::RangedInfo, NUM_HARDPOINTS > rangedWeapons;
	if ( parentChit->GetInventoryComponent() ) {
		parentChit->GetInventoryComponent()->GetRangedWeapons( &rangedWeapons );
	}

	currentAction = NO_ACTION;
	if ( !enemyList.Empty() ) {

		// Primary:   Distance: cubic, closer is better
		// Secondary: Strength: linear, more is better 
		// could add:
		//   damage done by enemy (higher is better)
		//   weakness to effects

		float bestUtility = 0;
		int   bestIndex = -1;
		int	  meleeInRange = 0;
		int	  bestAction = 0;

		for( int i=0; i<enemyList.Size(); ++i ) {
			Chit* enemyChit = GetChit( enemyList[i] );
			ComponentSet enemy( enemyChit, Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
			if ( enemy.okay ) 
			{
				const Vector3F enemyPos = enemy.spatial->GetPosition();
				float range = (enemyPos - pos).Length();
				if ( range < BattleMechanics::MeleeRange( parentChit, enemyChit ) ) {
					++meleeInRange;
				}
				float normalizedRange = range / LONGEST_WEAPON_RANGE;
				float utilityDistance = UtilityCubic( 1.0f, LOW_UTILITY, normalizedRange );

				float normalizedDamage = enemy.item->hp / thisComp.item->mass;	// basic melee advantage 
				float utilityDamage = UtilityLinear( 1.f, 0.f, normalizedDamage );

				float utility = (utilityDistance*PRIMARY_UTILITY + utilityDamage*SECONDARY_UTILITY)/(PRIMARY_UTILITY+SECONDARY_UTILITY);
				GLASSERT( utility >= 0 && utility <= 1.0f );
				if ( utility > bestUtility ) {
					bestIndex = i;
					bestUtility = utility;
					bestAction = MELEE;
				}
			}
		}
		// If there are melee targets in range, always go melee.
		// Else look for a shooting target that may be better.
		if ( !meleeInRange && rangedWeapons.Size() ) {
			// It would be nice to go through the loop twice. Integrate with above?
			for( int i=0; i<enemyList.Size(); ++i ) {

				const float bestRange = 10.0f;	// fixme: placeholder, depends on weapon
				// fixme: should account for:
				//		ranged weapons available (can be more than one?)
				//		whether needs re-load, or wait

				Chit* enemyChit = GetChit( enemyList[i] );
				ComponentSet enemy( enemyChit, Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
				if ( enemy.okay && LineOfSight( parentChit, enemy.chit )) {
					const Vector3F enemyPos = enemy.spatial->GetPosition();
					float range = (enemyPos - pos).Length();
					float normalizedRange = 0.5f * range / bestRange;
					float utility = UtilityParabolic( 0, 1, 0, normalizedRange );
					if ( utility > bestUtility ) {
						bestIndex = i;
						bestUtility = utility;
						bestAction = SHOOT;
					}
				}
			}
		}

		if ( bestIndex >= 0 ) {
			currentAction = bestAction;
			// Make the best target the 1st in the list.
			Swap( &enemyList[bestIndex], &enemyList[0] );
		}
		else {
			// Nothing with utility found. Clear out the enemy list,
			// so the AwareOfEnemy() will be false.
			enemyList.Clear();
		}
	}

#ifdef AI_OUTPUT
	static const char* actionName[NUM_ACTIONS] = { "noaction", "melee", "shoot" };
	GLOUTPUT(( "ID=%d Think: action=%s\n", parentChit->ID(), actionName[currentAction] ));
#endif
}


bool AIComponent::DoSlowTick()
{
	UpdateCombatInfo();
	return true;
}


bool AIComponent::DoTick( U32 deltaTime )
{
	// If we are in some action, do nothing and return.
	if (    parentChit->GetRenderComponent() 
		 && !parentChit->GetRenderComponent()->AnimationReady() ) 
	{
		return true;
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	if ( currentAction ) {

		switch( currentAction ) {

		case MELEE:		DoMelee();	break;
		case SHOOT:		DoShoot();	break;

		default:
			GLASSERT( 0 );
			currentAction = 0;
		}
	}
	return true;
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


// FIXME: move out of AI!
void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( chit == parentChit && msg.ID() == ChitMsg::RENDER_IMPACT ) {
		
		RenderComponent* render = parentChit->GetRenderComponent();
		GLASSERT( render );	// it is a message from the render component, after all.
		InventoryComponent* inventory = parentChit->GetInventoryComponent();
		GLASSERT( inventory );	// need to be  holding a melee weapon. possible the weapon
								// was lost before impact, in which case this assert should
								// be removed.

		IMeleeWeaponItem* item=inventory->GetMeleeWeapon();
		if ( render && inventory && item  ) { /* okay */ }
		else return;

		Matrix4 xform;
		render->GetMetaData( "trigger", &xform );
		Vector3F pos = xform * V3F_ZERO;

		engine->particleSystem->EmitPD( "derez", pos, V3F_UP, engine->camera.EyeDir3(), 0 );
		
		battleMechanics.MeleeAttack( engine, parentChit, item );
	}
}
