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

static const float NORMAL_AWARENESS		= 12.0f;

//#define AI_OUTPUT

AIComponent::AIComponent( Engine* _engine, WorldMap* _map ) : thinkTicker( 400 )
{
	engine = _engine;
	map = _map;
	currentAction = 0;
	currentTarget = 0;
	focusOnTarget = false;
	aiMode = NORMAL_MODE;
}


AIComponent::~AIComponent()
{
}


void AIComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, aiMode );
	XARC_SER( xs, currentAction );
	XARC_SER( xs, currentTarget );
	XARC_SER( xs, focusOnTarget );
	thinkTicker.Serialize( xs, "thinkTicker" );
	this->EndSerialize( xs );
}

int AIComponent::GetTeamStatus( Chit* other )
{
	// FIXME: placeholder friend/enemy logic
	ItemComponent* thisItem  = GET_COMPONENT( parentChit, ItemComponent );
	ItemComponent* otherItem = GET_COMPONENT( other, ItemComponent );
	if ( thisItem && otherItem ) {
		if ( thisItem->GetItem()->primaryTeam != otherItem->GetItem()->primaryTeam ) {
			return ENEMY;
		}
	}
	return FRIENDLY;
}


bool AIComponent::LineOfSight( const ComponentSet& thisComp, Chit* t )
{
	ComponentSet target( t, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );

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


static bool HasAI( Chit* c ) {
	return GET_COMPONENT( c, AIComponent ) != 0;
}

void AIComponent::GetFriendEnemyLists( const Rectangle2F* area )
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return;
	Vector2F center = sc->GetPosition2D();

	Rectangle2F zone;
	if ( area ) {
		zone = *area;
	}
	else {
		zone.min = zone.max = center;
		zone.Outset( NORMAL_AWARENESS );
	}

	friendList.Clear();
	enemyList.Clear();

	const CDynArray<Chit*>& chitArr = GetChitBag()->QuerySpatialHash( zone, parentChit, HasAI );
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
	if ( currentTarget && GetChitBag()->GetChit( currentTarget )) {
		int i = enemyList.Find( currentTarget );
		if ( i < 0 && enemyList.HasCap() ) {
			enemyList.Push( currentTarget );
		}
	}
}


Chit* AIComponent::Closest( const ComponentSet& thisComp, CArray<int, MAX_TRACK>* list )
{
	float best = FLT_MAX;
	Chit* chit = 0;

	for( int i=0; i<list->Size(); ++i ) {
		Chit* enemyChit = GetChit( *(list)[i] );
		ComponentSet enemy( enemyChit, Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
		if ( enemy.okay ) 
		{
			int len2 = (enemy.spatial->GetPosition() - thisComp.spatial->GetPosition()).LengthSquared();
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
	
}


void AIComponent::DoShoot( const ComponentSet& thisComp )
{
	bool pointed = false;
	Chit* targetChit = 0;

	// Use the current target, or find a new one.
	if ( !GetChitBag()->GetChit( currentTarget ) ) {

		float bestGolfScore = 1000.0f;

		for( int i=0; i<enemyList.Size(); ++i ) {
			ComponentSet target( GetChit( enemyList[0] ), Chit::RENDER_BIT | Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );
			if ( !target.okay ) {
				continue;
			}

			float 
		}
	}

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
					battleMechanics.Shoot(	GetChitBag(), 
											parentChit, targetChit, 
											weapons[i].weapon, weapons[i].trigger );
				}
				else {
					item->Reload();
				}
				break;
			}
		}
	}
}


void AIComponent::DoMelee( const ComponentSet& thisComp )
{
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
	if ( event.ID() == ChitEvent::AWARENESS ) {
		GetFriendEnemyLists( &event.AreaOfEffect() );
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


void AIComponent::ThinkWander( const ComponentSet& thisComp )
{

}


void AIComponent::ThinkBattle( const ComponentSet& thisComp )
{
	PathMoveComponent* pmc = GET_COMPONENT( thisComp.chit, PathMoveComponent );
	if ( !pmc ) {
		currentAction = NO_ACTION;
		return;
	}
	const Vector3F& pos = thisComp.spatial->GetPosition();
	
	// The current ranged weapon.
	CArray< InventoryComponent::RangedInfo, NUM_HARDPOINTS > rangedWeapons;
	thisComp.inventory->GetRangedWeapons( &rangedWeapons );

	// The current melee weapon.
	IMeleeWeaponItem* meleeWeapon = thisComp.inventory->GetMeleeWeapon();

	enum {
		OPTION_FLOCK_MOVE,
		OPTION_MOVE_TO_RANGE,
		OPTION_MELEE,
		OPTION_SHOOT,
		NUM_OPTIONS
	};

	float utility[NUM_OPTIONS] = { 0,0,0,0 };
	Chit* target[NUM_OPTIONS]  = { 0,0,0,0 };
	int rangedWeaponIndex      = -1;
	Vector2F moveToRange;

	// Consider flocking.
	Vector2F heading = thisComp.spatial->GetHeading2D();
	Vector2F flockDir = heading;
	utility[OPTION_FLOCK_MOVE] = CalcFlockMove( &flockDir );
	// Give this a little utility in case everything else is 0:
	utility[OPTION_FLOCK_MOVE] = Max( utility[OPTION_FLOCK_MOVE], 0.01f );

	for( int k=0; k<enemyList.Size(); ++k ) {

		ComponentSet enemy( enemyList[k], Chit::SPATIAL_BIT | Chit::ITEM_BIT | ComponentSet::IS_ALIVE );
		if ( !enemy.okay ) {
			continue;
		}
		const Vector3F enemyPos = enemy.spatial->GetPosition();
		float range = (enemyPos - pos).Length();
		Vector2F normalToEnemy = (enemyPos - pos).Normalize();
		float dot = DotProduct( normalToEnemy, heading );

		float q = 1.0f + dot;
		if ( enemyList[k] == currentTarget ) {
			q *= 2;
			if ( focusedOtTarget ) {
				q *= 2;
			}
		}

		// Consider ranged weapon options: OPTION_SHOOT, OPTION_MOVE_TO_RANGE
		for( int i=0; i<rangedWeapons.Size(); ++i ) {
			IRangedWeaponItem* pw = rangedWeapons[i].weapon;
			if ( pw->Ready() && pw->HasRound() ) {
				float effectiveRange = rangedWeapons[i].weapon->EffectiveRange();
				float u = 1.0f - fabs(range - effectiveRange) / effectiveRange; 
				u *= q;
				if ( currentAction == SHOOT ) {
					u *= 0.5f;
				} 

				if ( u > utility[OPTION_SHOOT] && LineOfSight( thisComp, enemy.chit ) ) {
					utility[OPTION_SHOOT] = u;
					target[OPTION_SHOOT] = enemy.chit;
					rangedWeaponIndex = i;
				}
			}
			// Close to effect range?
			float u = 1.0f - (range - effectiveRange ) / effectiveRange;
			u *= q;
			if ( pw->Ready() && pw->HasRound() ) {
				// okay;
			}
			else {
				u *= 0.5f;
			}
			if ( u > utility[OPTION_MOVE_TO_RANGE] ) {
				moveToRange = pos + enemyHeading * (range - effectiveRange);
				target[OPTION_MOVE_TO_RANGE] = enemy.chit;
			}
		}

		// Consider Melee
		if ( meleeWeapon ) {
			float u = BattleMechanics::MeleeRange( parentChit, enemy.chit ) / range;
			u *= q;
			if ( u > utility[MELEE_OPTION] ) {
				utility[OPTION_MELEE] = u;
				target[OPTION_MELEE] = enemy.chit;
			}
		}
	}

	int index = MaxValue( utility, NUM_OPTIONS );
	// Translate to action system:
	switch ( index ) {
		case OPTION_FLOCK_MOVE:
		{
			currentAction = MOVE;
			Vector2F dest = pos + flockDir;
			pmc->QueueDest( dest );
		}
		break;

		case OPTION_MOVE_TO_RANGE:
		{
			currentAction = MOVE;
			pmc->QueueDest( moveToRange );
		}
		break;

		case OPTION_MELEE:
		{
			currentAction = MELEE;
		}
		break;
		
		case OPTION_SHOOT:
		{
			currentAction = SHOOT;
		}
		break;

		default:
			GLASSERT( 0 );
	};

#ifdef AI_OUTPUT
	static const char* actionName[NUM_ACTIONS] = { "noaction", "melee", "shoot" };
	GLOUTPUT(( "ID=%d Think: action=%s\n", parentChit->ID(), actionName[currentAction] ));
#endif
}


int AIComponent::DoTick( U32 deltaTime, U32 timeSince )
{
	if ( thinkTicker.Delta( timeSince ) == 0 ) {
		return thinkTicker.Next();
	}
	ComponentSet thisComp( parentChit, Chit::RENDER_BIT | 
		                               Chit::SPATIAL_BIT |
									   Chit::INVENTORY_BIT |		// need to be carrying a melee weapon
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

	ChitBag* chitBag = this->GetChitBag();

	// If focused, make sure we have a target.
	if ( currentTarget && !chitBag->GetChit( currentTarget )) {
		currentTarget = 0;
	}
	if ( !currentTarget ) {
		focusOnTarget = false;
	}

	GetFriendEnemyLists( 0 );

	// High level mode switch?
	if ( aiMode == NORMAL_MODE && !enemyList.Empty() ) {
		aiMode = BATTLE_MODE;
		currentAction = 0;
	}
	else if ( aiMode == BATTLE_MODE && currentTarget == 0 && enemyList.Empty() ) {
		aiMode = NORMAL_MODE;
		currentAction = 0;
	}

	if ( !currentAction ) {
		Think( thisComp );
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	switch( currentAction ) {

		case MOVE:		DoMove( thisComp );		break;
		case MELEE:		DoMelee( thisComp );	break;
		case SHOOT:		DoShoot( thisComp );	break;

		default:
			GLASSERT( 0 );
			currentAction = 0;
			break;
	}
	return thinkTicker.Next();
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


