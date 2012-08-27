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

static const float PRIMARY_UTILITY		= 10.0f;
static const float SECONDARY_UTILITY	= 3.0f;
static const float TERTIARY_UTILITY		= 1.0f;

static const float LOW_UTILITY			= 0.2f;
static const float MED_UTILITY			= 0.5f;
static const float HIGH_UTILITY			= 0.8f;

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


void AIComponent::DoMelee()
{
	// FIXME: don't issue new move every tick?
	// FIXME: don't keep chasing when there are no targets

	// Are we close enough to hit? Then swing. Else move to target.
	Chit* targetChit = 0;
	while ( !enemyList.Empty() ) {
		targetChit = parentChit->GetChitBag()->GetChit( enemyList[0] );
		if ( targetChit )
			break;
		enemyList.SwapRemove(0);
	}
	if ( targetChit == 0 ) {
		return;
	}

	if ( battleMechanics.InMeleeZone( engine, parentChit, targetChit ) ) {
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
	}
	else {
		// Move to target.
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		if ( pmc ) {
			pmc->QueueDest( targetChit );
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

	ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT | Chit::ITEM_BIT );
	if ( !thisComp.okay )
		return;

	const Vector3F& pos = thisComp.spatial->GetPosition();

	if ( currentAction == NO_ACTION || currentAction == MELEE ) {
		currentAction = NO_ACTION;
		if ( !enemyList.Empty() ) {

			// Primary:   Distance: cubic, closer is better
			// Secondary: Strength: linear, more is better 
			// could add:
			//   damage done by enemy (higher is better)
			//   weakness to effects

			float bestUtility = 0;
			int   bestIndex = -1;

			for( int i=0; i<enemyList.Size(); ++i ) {
				ComponentSet enemy( GetChit( enemyList[i] ), Chit::SPATIAL_BIT | Chit::ITEM_BIT );
				if ( enemy.okay ) 
				{
					if ( enemy.item->hp == 0 )
						continue;	// already dead.

					const Vector3F enemyPos = enemy.spatial->GetPosition();
					float normalizedRange = (enemyPos - pos).Length() / COMBAT_INFO_RANGE;
					float utilityDistance = UtilityCubic( 1.0f, LOW_UTILITY, normalizedRange );

					float normalizedDamage = enemy.item->hp / thisComp.item->mass;	// basic melee advantage 
					float utilityDamage = UtilityLinear( 1.f, 0.f, normalizedDamage );

					float utility = utilityDistance*PRIMARY_UTILITY + utilityDamage*SECONDARY_UTILITY;
					if ( utility > bestUtility ) {
						bestIndex = i;
						bestUtility = utility;
					}
				}
			}
			if ( bestIndex >= 0 ) {
				currentAction = MELEE;
				// Make the best target the 1st in the list.
				Swap( &enemyList[bestIndex], &enemyList[0] );
			}
		}
	}
}


void AIComponent::DoSlowTick()
{
	UpdateCombatInfo();
}


void AIComponent::DoTick( U32 deltaTime )
{
	// If we are in some action, do nothing and return.
	if ( parentChit->GetRenderComponent() && !parentChit->GetRenderComponent()->AnimationReady() ) {
		return;
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	if ( currentAction ) {

		switch( currentAction ) {

		case MELEE:
			DoMelee();
			break;

		default:
			GLASSERT( 0 );
			currentAction = 0;
		}
	}
	else {
		Think();
	}
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


// FIXME: move out of AI!
void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( chit == parentChit && msg.ID() == RENDER_MSG_IMPACT ) {
		
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
