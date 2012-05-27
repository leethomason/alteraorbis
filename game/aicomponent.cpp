#include "aicomponent.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"
#include "gameitem.h"
#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../grinliz/glrectangle.h"
#include <climits>

using namespace grinliz;

static const U32	UPDATE_COMBAT_INFO	= 1000;		// how often to update the friend/enemy lists
static const float	COMBAT_INFO_RANGE	= 10.0f;	// range around to scan for friendlies/enemies


AIComponent::AIComponent( WorldMap* _map, int _team )
{
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


void AIComponent::DoTick( U32 delta )
{
	// Update the info around us.
	// Then:
	//		Move: closer, away, strafe
	//		Shoot
	//		Reload

	// Routine update to situational awareness.
	combatInfoAge += delta;
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
			UpdateCombatInfo( &events[i].bounds );
		}
	}

	if ( enemyList.Size() > 1 ) {
		PathMoveComponent* pmc = GET_COMPONENT( parentChit, PathMoveComponent );
		if ( pmc ) pmc->QueueDest( enemyList[0].chit->GetSpatialComponent()->GetPosition2D() );

		grinliz::CArray<XEItem*, MAX_ACTIVE_ITEMS> activeItems;
		GameItem::GetActiveItems( parentChit, &activeItems );
	}
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}

