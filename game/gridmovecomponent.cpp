#include "gridmovecomponent.h"
#include "worldinfo.h"
#include "gamelimits.h"
#include "worldmap.h"
#include "pathmovecomponent.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

static const float GRID_ACCEL = 1.0f;

GridMoveComponent::GridMoveComponent() : GameMoveComponent()
{
	state = NOT_INIT;
	velocity.Zero();
	speed = DEFAULT_MOVE_SPEED;
}


GridMoveComponent::~GridMoveComponent()
{
}


void GridMoveComponent::DebugStr( grinliz::GLString* str )
{
	static const char* STATE[] = { "NOT_INIT", "ON_BOARD", "TRAVELLING", "OFF_BOARD", "DONE" };
	str->AppendFormat( "[GridMove state=%s] ", STATE[state] );
}


void GridMoveComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, state );
	XARC_SER( xs, destSectorPort.sector );
	XARC_SER( xs, destSectorPort.port );
	XARC_SER( xs, speed );

	this->EndSerialize( xs );
}


void GridMoveComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
}


void GridMoveComponent::OnRemove()
{
	super::OnRemove();
}


bool GridMoveComponent::IsMoving() const
{
	// This is something of a problem. Are we moving
	// if on transit? Mainly used by the animation system,
	// so the answer is 'no', but not quite what you
	// might expect.
	return false;
}


void GridMoveComponent::SetDest( const SectorPort& sp )
{
	switch( state ) {
	case NOT_INIT:
	case ON_BOARD:
	case OFF_BOARD:
		state = ON_BOARD;
		break;
	case TRAVELLING:
		path.Clear();
		break;
	}

	destSectorPort = sp;
	// Error check:
	//const ChitContext* context = this->GetChitContext();
	//context->worldMap->GetWorldInfo().GetGridEdge( destSectorPort.sector, destSectorPort.port );
}


int GridMoveComponent::DoTick( U32 delta )
{
	const ChitContext* context = Context();
	if ( state == DONE ) {
		parentChit->Swap( this, new PathMoveComponent());
		return VERY_LONG_TICK;
	}
	if ( state == NOT_INIT ) {
		return VERY_LONG_TICK;
	}

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return VERY_LONG_TICK;

	int tick = 0;
	Vector2F pos = sc->GetPosition2D();

	// Speed up or slow down, depending
	// on on or off board.
	if ( state != OFF_BOARD ) {
		if ( speed < GRID_SPEED ) {
			speed += Travel( GRID_ACCEL, delta );
			if ( speed > GRID_SPEED ) speed = GRID_SPEED;
		}
	}
	else {
		speed -= Travel( GRID_ACCEL, delta );
		if ( speed < DEFAULT_MOVE_SPEED ) speed = DEFAULT_MOVE_SPEED;
	}

	Vector2F dest = { 0, 0 };
	int stateIfDestReached = DONE;
	bool goToDest = true;
	const WorldInfo& worldInfo = context->worldMap->GetWorldInfo();
	GLASSERT( &worldInfo );

	switch ( state ) 
	{
	
	case ON_BOARD:
		// Presume we started somewhere rational. Move to grid.
		{
			const SectorData& sd = worldInfo.GetSectorInfo( pos.x, pos.y );
			int port = sd.NearestPort( pos );
			Vector2I sector = { sd.x / SECTOR_SIZE, sd.y / SECTOR_SIZE };
			GridEdge gridEdge = worldInfo.GetGridEdge( sector, port );
			GLASSERT( worldInfo.HasGridEdge( gridEdge ));

			dest = worldInfo.GridEdgeToMapF( gridEdge );
			stateIfDestReached = TRAVELLING;
		}
		break;

	case OFF_BOARD:
		{
			GLASSERT( destSectorPort.IsValid() );
			const SectorData& sd = worldInfo.GetSector( destSectorPort.sector );
			Rectangle2I portBounds = sd.GetPortLoc( destSectorPort.port );
			dest = SectorData::PortPos( portBounds, parentChit->ID() );
			stateIfDestReached = DONE;
		}
		break;

	case TRAVELLING:
		{	
			GLASSERT( destSectorPort.IsValid() );
			GridEdge destEdge = worldInfo.GetGridEdge( destSectorPort.sector, destSectorPort.port );
			GLASSERT( worldInfo.HasGridEdge( destEdge ));
			Vector2F destPt   = worldInfo.GridEdgeToMapF( destEdge ); 
			
			Rectangle2F destRect;
			destRect.min = destRect.max = destPt;
			destRect.Outset( 0.2f );

			goToDest = false;
			if ( destRect.Contains( pos )) {
				state = OFF_BOARD;
			}
			else {
				if ( path.Size() == 0 ) {
					Vector2I mapPos = { (int)pos.x, (int)pos.y };
					GridEdge current = context->worldMap->GetWorldInfo().MapToGridEdge( mapPos.x, mapPos.y );
					GLASSERT( context->worldMap->GetWorldInfo().HasGridEdge( current ));
					int result = context->worldMap->GetWorldInfoMutable()->Solve( current, destEdge, &path );
					if ( result == micropather::MicroPather::START_END_SAME ) {
						path.Clear();
						path.Push( current );
					}
					else if ( result != micropather::MicroPather::SOLVED ) {
						path.Clear();
						path.Push( destEdge );
					}
					else {
						GLASSERT( result == micropather::MicroPather::SOLVED );
					}
				}

				float travel = Travel( speed, delta );
				while ( travel > 0 && !path.Empty() ) {
					GridEdge ge   = path[0];
					Vector2F target = worldInfo.GridEdgeToMapF( ge );

					Vector2F delta = target - pos;
					float len = delta.Length();
					if ( len <= travel ) {
						travel -= len;
						pos = target;
						path.Remove( 0 );	// FIXME: potentially expensive - use index
					}
					else {
						Vector2F dir = { Sign( delta.x ), Sign( delta.y ) };
						pos = pos + dir * travel;
						travel = 0;
					}
				}

			}
		}
		break;

	};

	if ( goToDest ) {
		Vector2F dir = dest - pos;
		float distance = dir.Length();
		if ( distance > 0 ) {
			dir.Multiply( 1.0f / distance );	// Normalize.

			float travel = Travel( speed, delta );
			velocity.Set( dir.x*speed, 0, dir.y*speed );

			if ( travel > distance ) travel = distance;
			pos = pos + dir * travel;
			if ( travel >= distance ) {
				state = stateIfDestReached;
				pos = dest;
			}
		}
		else {
			state = stateIfDestReached;
		}
	}
	GLASSERT( sc );
	sc->SetPosition( pos.x, 0, pos.y );
	return tick;
}
