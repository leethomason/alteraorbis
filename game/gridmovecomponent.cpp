#include "gridmovecomponent.h"
#include "worldmap.h"
#include "pathmovecomponent.h"
#include "lumoschitbag.h"

#include "../xegame/chitcontext.h"

using namespace grinliz;

static const float GRID_ACCEL = 1.0f;


GridMoveComponent::GridMoveComponent() : GameMoveComponent()
{
	state = NOT_INIT;
	velocity.Zero();
	speed = DEFAULT_MOVE_SPEED;
	lastGB.Zero();
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
	if (state == TRAVELLING) {
		if (Context() && parentChit) {

			GLASSERT(Context()->worldMap->GetWorldGrid(ToWorld2I(parentChit->Position())).IsGrid());
		}
	}

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

		default:
			GLASSERT(false);
			break;
	}

	destSectorPort = sp;
	GLASSERT(sp.IsValid());
	// Error check:
	//const ChitContext* context = this->GetChitContext();
	//context->worldMap->GetWorldInfo().GetGridEdge( destSectorPort.sector, destSectorPort.port );
}


int GridMoveComponent::DoTick( U32 delta )
{
	const ChitContext* context = Context();
	if ( state == DONE ) {
		GLASSERT(parentChit->StackedMoveComponent());
		Context()->chitBag->QueueRemoveAndDeleteComponent(this);
		return VERY_LONG_TICK;
	}
	if ( state == NOT_INIT ) {
		return VERY_LONG_TICK;
	}

	int tick = 0;
	Vector2F pos = ToWorld2F(parentChit->Position());

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
			GLASSERT(port);
			Vector2I sector = sd.sector;
			GridBlock gridBlock = worldInfo.GetGridBlock( sector, port );
			GLASSERT(!gridBlock.IsZero());

			dest.Set((float)gridBlock.x, (float)gridBlock.y);
			stateIfDestReached = TRAVELLING;
		}
		break;

	case OFF_BOARD:
		{
			GLASSERT( destSectorPort.IsValid() );
			const SectorData& sd = worldInfo.GetSector( destSectorPort.sector );
			Rectangle2I portBounds = sd.GetPortLoc( destSectorPort.port );
			dest = SectorData::PortPos( portBounds, (U32) parentChit->ID());
			stateIfDestReached = DONE;
		}
		break;

	case TRAVELLING:
		{	
			GLASSERT( destSectorPort.IsValid() );
			GridBlock destBlock = worldInfo.GetGridBlock( destSectorPort.sector, destSectorPort.port );
			GLASSERT(!destBlock.IsZero());
			Vector2F destPt = { (float)destBlock.x, (float)destBlock.y };
			GLASSERT(Context()->worldMap->GetWorldGrid(ToWorld2I(parentChit->Position())).IsGrid());
			
			Rectangle2F destRect;
			destRect.min = destRect.max = destPt;
			destRect.Outset( 0.2f );

			goToDest = false;
			if ( destRect.Contains( pos )) {
				state = OFF_BOARD;
			}
			else {
				if ( path.Size() == 0 ) {
					GridBlock current = MapToGridBlock(pos.x, pos.y);
					lastGB.Zero();

					int result = context->worldMap->GetWorldInfoMutable()->Solve( current, destBlock, &path );
					if ( result == micropather::MicroPather::START_END_SAME ) {
						path.Clear();
						path.Push( current );
					}
					else if ( result != micropather::MicroPather::SOLVED ) {
						path.Clear();
						path.Push(destBlock);
					}
					else {
						GLASSERT( result == micropather::MicroPather::SOLVED );
					}
				}

				float travel = Travel( speed, delta );
				while ( travel > 0 && !path.Empty() ) {
					GridBlock gb   = path[0];
					Vector2F target = { (float)gb.x, (float)gb.y };

					/* This would, in theory, add lanes. But not working.
					if (!lastGB.IsZero()) {
						int dx = gb.x - lastGB.x;
						int dy = gb.y - lastGB.y;
						if (dx && !dy) {
							target.y += -0.5f * Sign((float)dx);
						}
						else if (!dx && dy) {
							target.x += 0.5f * Sign((float)dy);
						}
						GLASSERT(Context()->worldMap->GetWorldGrid(ToWorld2I(target)).IsGrid());
					}
					*/

					Vector2F delta = target - pos;
					float len = delta.Length();
					if ( len <= travel ) {
						travel -= len;
						pos = target;
						lastGB = path[0];
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

		default: GLASSERT(false); break;
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
	parentChit->SetPosition( pos.x, 0, pos.y );
	return tick;
}
