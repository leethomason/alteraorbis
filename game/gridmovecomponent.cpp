#include "gridmovecomponent.h"
#include "worldinfo.h"
#include "gamelimits.h"
#include "worldmap.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

using namespace grinliz;

static const float GRID_SPEED = MOVE_SPEED * 2.0f;
static const float GRID_ACCEL = 1.0f;

GridMoveComponent::GridMoveComponent( WorldMap* wm ) : GameMoveComponent( wm )
{
	worldMap = wm;
	state = NOT_INIT;
}


GridMoveComponent::~GridMoveComponent()
{
}


void GridMoveComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[GridMove] " );
}


void GridMoveComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	GLASSERT( 0 );
	this->EndSerialize( xs );
}


void GridMoveComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
}


void GridMoveComponent::OnRemove()
{
	super::OnRemove();
}


bool GridMoveComponent::IsMoving() const
{
	return true;
}


GridEdge GridMoveComponent::ToGridEdge( int sectorX, int sectorY, int port )
{
	GridEdge g = { -1, -1, -1 };
	if ( port == SectorData::NEG_X ) {
		g.x = sectorX;
		g.y = sectorY;
		g.alignment = GridEdge::HORIZONTAL;
	}
	else if ( port == SectorData::POS_X ) {
		g.x = sectorX+1;
		g.y = sectorY;
		g.alignment = GridEdge::HORIZONTAL;
	}
	else if ( port == SectorData::NEG_Y ) {
		g.x = sectorX;
		g.y = sectorY;
		g.alignment = GridEdge::VERTICAL;
	}
	else if ( port == SectorData::POS_Y ) {
		g.x = sectorX;
		g.y = sectorY+1;
		g.alignment = GridEdge::VERTICAL;
	}
	return g;
}


void GridMoveComponent::SetDest( int sectorX, int sectorY, int port )
{
	GLASSERT( state == NOT_INIT );

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	GLASSERT( sc );
	if ( !sc ) return;

	Vector2F pos = sc->GetPosition2D();
	int axis = 0;
	Vector2I sectorPos = SectorData::SectorID( pos.x, pos.y, &axis );

	WorldInfo* worldInfo = worldMap->GetWorldInfoMutable();
	current = ToGridEdge( sectorPos.x, sectorPos.y, axis );
	if ( !worldInfo->HasGridEdge( current )) {
		GLASSERT( 0 );
		return;
	}

	sectorDest.Set( sectorX, sectorY );
	portDest = port;
	GridEdge dest = ToGridEdge( sectorX, sectorY, port );
	if ( !worldInfo->HasGridEdge( dest )) {
		GLASSERT( 0 );
		return;
	}

	int result = worldInfo->Solve( current, dest, &path );
	GLASSERT( result == micropather::MicroPather::START_END_SAME || 
			  result == micropather::MicroPather::SOLVED );

	state = ON_BOARD;

	RenderComponent* rc = parentChit->GetRenderComponent();
	if ( rc ) {
		rc->PlayAnimation( ANIM_STAND );
	}
	parentChit->SetTickNeeded();
}


int GridMoveComponent::DoTick( U32 delta, U32 since )
{
	if ( state == DONE ) {
		if ( deleteWhenDone ) {
			parentChit->Remove( this );
			delete this;
		}
		return VERY_LONG_TICK;
	}
	if ( state == NOT_INIT ) {
		return VERY_LONG_TICK;
	}

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if ( !sc ) return VERY_LONG_TICK;
	RenderComponent* rc = parentChit->GetRenderComponent();

	int tick = 0;
	Vector2F pos = sc->GetPosition2D();

	if ( state != OFF_BOARD ) {
		if ( speed < GRID_SPEED ) {
			speed += Travel( GRID_ACCEL, since );
			if ( speed > GRID_SPEED ) speed = GRID_SPEED;
		}
	}
	else {
		speed -= Travel( GRID_ACCEL, since );
		if ( speed < 0 ) speed = 0;
	}

	switch ( state ) 
	{
	case ON_BOARD:
		// Presume we started somewhere rational. Move to grid.
		{
			Vector2F dir = { 0, 0 };
			Vector2F center = current.Center();
			float distance = 0;
			if ( current.alignment == GridEdge::HORIZONTAL ) {
				distance = fabs( center.y - pos.y );
				if ( center.y < pos.y ) dir.Set( 0, -1 );
				else dir.Set( 0, 1 );
			}
			else {
				distance = fabs( center.x - pos.y );
				if ( center.x < pos.x ) dir.Set( -1, 0 );
				else dir.Set( 1, 0 );
			}
			float travel = Travel( speed, since );
			if ( travel > distance ) travel = distance;
			pos = pos + dir * travel;
			if ( travel >= distance ) {
				//state = TRAVELLING;
				state = DONE;
			}
		}
		break;
	};
	return tick;
}
