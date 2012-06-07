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

#include "../grinliz/glrectangle.h"
#include <climits>

using namespace grinliz;

static const U32	UPDATE_COMBAT_INFO	= 1000;		// how often to update the friend/enemy lists
static const float	COMBAT_INFO_RANGE	= 10.0f;	// range around to scan for friendlies/enemies


AIComponent::AIComponent( Engine* _engine, WorldMap* _map, int _team )
{
	enabled = true;
	engine = _engine;
	map = _map;
	team = _team;
	combatInfoAge = 0xffffff;
}


AIComponent::~AIComponent()
{
}


int AIComponent::GetTeamStatus( const AIComponent* other )
{
	if ( other->team == this->team )
		return FRIENDLY;
	return ENEMY;
}


void AIComponent::UpdateChitData()
{
	for( int k=0; k<2; ++k ) {
		CArray<ChitData, MAX_TRACK>& list = (k==0) ? friendList : enemyList;

		unsigned i=0; 
		while( i < list.Size() ) {
			Chit* chit = GetChitBag()->GetChit( list[i].chitID );
			list[i].chit = chit;
			if ( chit && chit->GetSpatialComponent() ) {
				list[i].range = ( parentChit->GetSpatialComponent()->GetPosition() - chit->GetSpatialComponent()->GetPosition() ).Length();
				++i;
			}
			else {
				list.SwapRemove( i );
			}
		}
	}
}


void AIComponent::UpdateCombatInfo( const Rectangle2F* _zone )
{
	combatInfoAge = (parentChit->ID())%100;	// space out updates in a random yet predictable way

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return;
	Vector2F center = sc->GetPosition2D();

	Rectangle2F zone;
	zone.min = center; zone.max = center;
	zone.Outset( COMBAT_INFO_RANGE );
	if ( !_zone ) {
		// Clear and reset the existing info.
		friendList.Clear();
		enemyList.Clear();
	}
	else {
		// Use the passed in info, add to the existing.
		zone = *_zone;
	}

	const CDynArray<Chit*>& chitArr = GetChitBag()->QuerySpatialHash( zone, parentChit, true );

	// Sort in by as-the-crow-flies range. Not correct, but don't want to deal with arbitrarily long query.

	for( int i=0; i<chitArr.Size(); ++i ) {
		Chit* chit = chitArr[i];
		AIComponent* ai = GET_COMPONENT( chit, AIComponent );
		if ( ai ) {
			int teamStatus = GetTeamStatus( ai );
			ChitData* cd = 0;

			if ( teamStatus == FRIENDLY && friendList.HasCap() )
				cd = friendList.Push();
			else if ( teamStatus == ENEMY && enemyList.HasCap() )
				cd = enemyList.Push();

			if ( cd ) {
				Vector2F chitCenter = chit->GetSpatialComponent()->GetPosition2D();
				float cost = FLT_MAX;
				map->CalcPath( center, chitCenter, 0, 0, 0, &cost, false );

				cd->chitID = chit->ID();
				cd->chit   = chit;
				cd->pathDistance = cost;
				cd->range = (center-chitCenter).Length();
			}
		}
	}
}


void AIComponent::DoTick( U32 deltaTime )
{
	if ( !enabled ) 
		return;

	// Update the info around us.
	// Then:
	//		Move: closer, away, strafe
	//		Shoot
	//		Reload

	// Routine update to situational awareness.
	combatInfoAge += deltaTime;
	if ( combatInfoAge > UPDATE_COMBAT_INFO ) {
		UpdateCombatInfo();
	}
	else {
		UpdateChitData();
	}

	// Check for events that change the situation
	const CDynArray<ChitEvent>& events = GetChitBag()->GetEvents();
	for( int i=0; i<events.Size(); ++i ) {
		if(    events[i].id == AI_EVENT_AWARENESS 
			&& events[i].data0 == team ) 
		{
			// FIXME: double call can cause 2 entries for the same unit
			UpdateCombatInfo( &events[i].bounds );
		}
	}

	if ( enemyList.Size() > 0 ) {
		const ChitData& target = enemyList[0];

		SpatialComponent*  thisSpatial = parentChit->GetSpatialComponent();
		RenderComponent*   thisRender = parentChit->GetRenderComponent();
		SpatialComponent*  targetSpatial = target.chit->GetSpatialComponent();
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		U32 absTime = GetChitBag()->AbsTime();

		grinliz::CArray<XEItem*, MAX_ACTIVE_ITEMS> activeItems;
		GameItem::GetActiveItems( parentChit, &activeItems );
		// FIXME: choose weapons, etc.
		XEItem* xeitem = activeItems[0];
		GameItem* gameItem = xeitem->ToGameItem();
		WeaponItem* weapon = gameItem->ToWeapon();

		GLASSERT( pmc );
		GLASSERT( thisSpatial );
		GLASSERT( targetSpatial );
		GLASSERT( thisRender );

		static const Vector3F UP = { 0, 1, 0 };
		const Vector3F* eyeDir = engine->camera.EyeDir3();

		if ( activeItems.Size() > 0 ) {
			// fixme: use best item for situation

			if (    BattleMechanics::InMeleeZone( thisSpatial->GetPosition2D(),
												  thisSpatial->GetHeading2D(),
												  targetSpatial->GetPosition2D() ))
			{
				if ( weapon->CanMelee( absTime ) )
				{
					BattleMechanics::MeleeAttack( parentChit, weapon );
																												
				}
				// fixme: else queue rotation.
			}
			else {
				if ( pmc ) {
					pmc->QueueDest( enemyList[0].chit->GetSpatialComponent()->GetPosition2D() );
				}
			}
		}
	}
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}


void AIComponent::OnChitMsg( Chit* chit, int id, const ChitEvent* event )
{
}
