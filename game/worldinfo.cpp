#include "worldinfo.h"

#include "../grinliz/glrandom.h"

using namespace tinyxml2;
using namespace grinliz;

WorldInfo::WorldInfo()
{
	pather = new micropather::MicroPather( this, NUM_SECTORS*NUM_SECTORS, 4, true );
}


WorldInfo::~WorldInfo()
{
	delete pather;
}


int WorldInfo::Solve( GridEdge start, GridEdge end, grinliz::CDynArray<GridEdge>* path )
{
	GLASSERT( HasGridEdge( start ));
	GLASSERT( HasGridEdge( end ));

	path->Clear();
	float cost = 0;
	int result = pather->Solve( ToState( start ), ToState( end ), &patherVector, &cost );
	for( unsigned i=0; i<patherVector.size(); ++i ) {
		path->Push( FromState( patherVector[i] ));
	}
	return result;
}


void WorldInfo::Serialize( XStream* xs )
{
	XarcOpen( xs, "WorldInfo" );

	for( int i=0; i<NUM_SECTORS*NUM_SECTORS; ++i ) {
		sectorData[i].Serialize( xs );
	}

	XarcClose( xs );
}


bool WorldInfo::HasGridEdge( GridEdge g ) const
{
	if ( g.x < 0 || g.x >= NUM_SECTORS || g.y < 0 || g.y >= NUM_SECTORS )
		return false;
	const SectorData& sd = sectorData[g.y+NUM_SECTORS+g.x];

	// GridEdge H at 0,0 and V at 0,0 is the origin.
	if ( g.alignment == GridEdge::HORIZONTAL ) {
		return ( sd.ports & SectorData::NEG_Y ) != 0;
	}
	return ( sd.ports & SectorData::NEG_X ) != 0;
}


GridEdge WorldInfo::GetGridEdge( const grinliz::Vector2I& sector, int axis ) const
{
	GLASSERT( sector.x >= 0 && sector.x < NUM_SECTORS && sector.y >= 0 && sector.y < NUM_SECTORS );
	const SectorData& sd = sectorData[sector.y*NUM_SECTORS+sector.x];
	GridEdge g = { 0, 0, 0 };

	if ( sd.ports & axis ) {
		if ( axis == SectorData::NEG_X ) {
			g.alignment = GridEdge::VERTICAL;
			g.x = sector.x;
			g.y = sector.y;
		}
		else if ( axis == SectorData::POS_X ) {
			g.alignment = GridEdge::VERTICAL;
			g.x = sector.x + 1;
			g.y = sector.y;
		}
		else if ( axis == SectorData::NEG_Y ) {
			g.alignment = GridEdge::HORIZONTAL;
			g.x = sector.x;
			g.y = sector.y;
		}
		else if ( axis == SectorData::POS_Y ) {
			g.alignment = GridEdge::HORIZONTAL;
			g.x = sector.x;
			g.y = sector.y + 1;
		}
		else {
			GLASSERT( 0 );
		}
	}
	GLASSERT( HasGridEdge( g ));
	return g;
}


GridEdge WorldInfo::GetGridEdge( float x, float y ) const
{
	int gx = (int)x / SECTOR_SIZE;
	int gy = (int)y / SECTOR_SIZE;
	int align = 0;

	// Horizontal.
	if ( y == (float)(gy*SECTOR_SIZE) ) {
		align = GridEdge::HORIZONTAL;
	}
	else {
		align = GridEdge::VERTICAL;
	}
	GridEdge ge = { align, gx, gy };
	GLASSERT( HasGridEdge( ge ));
	return ge;
}


int WorldInfo::NearestPort( const grinliz::Vector2I& sector, const grinliz::Vector2F& p ) const
{
	const SectorData& sd = GetSector( sector.x, sector.y );
	int best = 0;
	if ( sd.ports ) {
		int shortest = INT_MAX;

		for( int i=0; i<4; ++i ) {
			int port = 1<<i;
			if ( sd.ports & port ) {
				int len2 = (sd.GetPortLoc(port).Center() - sector).LengthSquared();
				if ( len2 < shortest ) {
					shortest = len2;
					best = port;
				}
			}
		}
	}
	return best;
}


const SectorData& WorldInfo::GetSectorInfo( float fx, float fy, int *p_axis, GridEdge* p_edge ) const
{
	const SectorData& sd = GetSector( (int)fx, (int)fy );
	Vector2I v = { sd.x/SECTOR_SIZE, sd.y/SECTOR_SIZE };

	int axis = 0;
	if ( p_axis || p_edge ) {
		int dx = (int)fx - (v.x*SECTOR_SIZE + SECTOR_SIZE/2);
		int dy = (int)fy - (v.y*SECTOR_SIZE + SECTOR_SIZE/2);
		if ( abs(dx) > abs(dy) ) {
			axis = ( dx > 0 ) ? SectorData::POS_X : SectorData::NEG_X;
		}
		else {
			axis = ( dy > 0 ) ? SectorData::POS_Y : SectorData::NEG_Y;
		}
	}
	if ( p_edge && axis ) {
		p_edge->x = 0;
		p_edge->y = 0;
		*p_edge = GetGridEdge( v, axis );
	}
	if ( p_axis ) {
		*p_axis = axis;
	}
	return sd;
}


float WorldInfo::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	GridEdge start = FromState( stateStart );	
	GridEdge end   = FromState( stateEnd );

	GLASSERT( HasGridEdge( start ));
	GLASSERT( HasGridEdge( end ));

	Vector2F cStart = start.GridCenter();
	Vector2F cEnd   = end.GridCenter();

	float len = fabs( cStart.x - cEnd.x ) + fabs( cStart.y - cEnd.y );
	return len;
}


void  WorldInfo::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	adjacent->clear();
	const GridEdge g = FromState( state );
	GLASSERT( HasGridEdge( g ));

	/*
	 	  |	V1,0  |    V2,0
	 	--+-------+--- H2,1
	H 0,1 |	1,1   |    V2,1
	V 1,1
	*/
	const GridEdge alth[6] = {
		{ GridEdge::VERTICAL,   g.x,   g.y-1 },
		{ GridEdge::HORIZONTAL, g.x-1, g.y },
		{ GridEdge::VERTICAL,   g.x,   g.y },

		{ GridEdge::VERTICAL,	g.x+1, g.y-1 },
		{ GridEdge::HORIZONTAL,	g.x+1, g.y },
		{ GridEdge::VERTICAL,	g.x+1, g.y } 
	};

	/*
		|V1,0
   H0,1-+- H1,1
		|
		| 1,1
		|
		+

	*/

	const GridEdge altv[6] = {
		{ GridEdge::HORIZONTAL, g.x-1, g.y },
		{ GridEdge::VERTICAL,	g.x,   g.y-1 },
		{ GridEdge::HORIZONTAL, g.x,   g.y },

		{ GridEdge::HORIZONTAL,	g.x-1, g.y+1 },
		{ GridEdge::VERTICAL,	g.x,   g.y+1 },
		{ GridEdge::HORIZONTAL,	g.x,   g.y+1 } 
	};

	const GridEdge* alt = ( g.alignment == GridEdge::HORIZONTAL ) ? alth : altv;

	for( int i=0; i<6; ++i ) {
		if ( HasGridEdge( alt[i] )) {
			micropather::StateCost sc = { ToState(alt[i]), 1.0f };
			adjacent->push_back( sc );
		}
	}
}


void  WorldInfo::PrintStateInfo( void* state )
{
	GridEdge v = FromState( state );
	GLOUTPUT(( "%s %d,%d ", v.alignment == GridEdge::HORIZONTAL ? "H" : "V", v.x, v.y ));
}

