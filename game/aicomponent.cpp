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

static const U32	UPDATE_COMBAT_INFO	= 1000;		// how often to update the friend/enemy lists
static const float	COMBAT_INFO_RANGE	= 10.0f;	// range around to scan for friendlies/enemies


AIComponent::AIComponent( Engine* _engine, WorldMap* _map )
{
	engine = _engine;
	map = _map;
	currentAction = 0;
}


AIComponent::~AIComponent()
{
}


/*
bool AIComponent::AwareOfEnemy() const {
	for( int i=0; i<enemyList.Size(); ++i ) {
		Chit* chit = parentChit->GetChitBag()->GetChit( enemyList[i] );
		if ( chit )
			return true;
	}
	return false;
}
*/


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
	GetChitBag()->QuerySpatialHash( &chitArr, zone, parentChit, GameItem::CHARACTER, true );

	if ( !chitArr.Empty() ) {
		// Clear and reset the existing info.
		friendList.Clear();
		enemyList.Clear();

		for( int i=0; i<chitArr.Size(); ++i ) {
			Chit* chit = chitArr[i];

			int teamStatus = GetTeamStatus( chit );

			if ( teamStatus == FRIENDLY && friendList.HasCap() )
				friendList.Push( chit->ID() );
			else if ( teamStatus == ENEMY && enemyList.HasCap() )
				 enemyList.Push( chit->ID() );
		}
	}
}


void AIComponent::DoSlowTick()
{
	UpdateCombatInfo();
}


void AIComponent::DoMelee()
{
	// Are we close enough to hit? Then swing. Else move to target.

	Chit* targetChit = parentChit->GetChitBag()->GetChit( action.melee.targetID );
	if ( targetChit == 0 ) {
		currentAction = NO_ACTION;
		if ( enemyList.Size() && enemyList[0] == action.melee.targetID ) {
			enemyList.PopFront();
		}
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
	if ( event.ID() == ChitEvent::AWARENESS ) {
		ItemComponent* itemComp = GET_COMPONENT( parentChit, ItemComponent );
		if ( itemComp ) {
			const AwarenessChitEvent& aware = (const AwarenessChitEvent&) event;
			if ( aware.Team() == itemComp->item.primaryTeam ) 
			{
				UpdateCombatInfo( &aware.Bounds() );
			}
		}
	}
}


void AIComponent::DoTick( U32 deltaTime )
{
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
		
		if ( !enemyList.Empty() ) {
			currentAction = MELEE;
			action.melee.targetID = enemyList[0];
		}
	}
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


void AIComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( chit == parentChit && msg.ID() == RENDER_MSG_IMPACT ) {
		
		RenderComponent* render = parentChit->GetRenderComponent();
		GLASSERT( render );	// it is a message from the render component, after all.
		InventoryComponent* inventory = parentChit->GetInventoryComponent();
		GLASSERT( inventory );	// need to be  holding a melee weapon. possible the weapon
								// was lost before impact, in which case this assert should
								// be removed.

		CArray< GameItem*, EL_MAX_METADATA > weapons;
		inventory->GetWeapons( &weapons );
		GameItem* item=0;
		if ( weapons.Size() ) item = weapons[0];

		if ( render && inventory && item && item->ToMeleeWeapon() ) { /* okay */ }
		else return;

		Matrix4 xform;
		render->GetMetaData( "trigger", &xform );
		Vector3F pos = xform * V3F_ZERO;

		engine->particleSystem->EmitPD( "derez", pos, V3F_UP, engine->camera.EyeDir3(), 0 );
		
		battleMechanics.MeleeAttack( engine, parentChit, item->ToMeleeWeapon() );
	}
}
