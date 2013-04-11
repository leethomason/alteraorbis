#include "worldinfo.h"
#include "worldgrid.h"

#include "../grinliz/glrandom.h"

using namespace tinyxml2;
using namespace grinliz;

WorldInfo::WorldInfo( const WorldGrid* grid, int mw, int mh )
{
	pather = new micropather::MicroPather( this, NUM_SECTORS*NUM_SECTORS, 4, true );
	worldGrid = grid;
	mapWidth = mw;
	mapHeight = mh;
}


WorldInfo::~WorldInfo()
{
	delete pather;
}


int WorldInfo::Solve( GridEdge start, GridEdge end, grinliz::CDynArray<GridEdge>* path )
{
	GLASSERT( HasGridEdge( start.x, start.y ));
	GLASSERT( HasGridEdge( end.x, end.y ));

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


bool WorldInfo::HasGridEdge( int geX, int geY ) const
{
	Vector2I m = GridEdgeToMap( geX, geY );
	return worldGrid[m.y*mapWidth + m.x].IsGrid();
}


GridEdge WorldInfo::GetGridEdge( const grinliz::Vector2I& sector, int port ) const
{
	GLASSERT( sector.x >= 0 && sector.x < NUM_SECTORS && sector.y >= 0 && sector.y < NUM_SECTORS );
	const SectorData& sd = sectorData[sector.y*NUM_SECTORS+sector.x];
	GridEdge g = { 0, 0 };

	if ( sd.ports & port ) {
		if ( port == SectorData::NEG_X ) {
			g.x = sector.x*2;
			g.y = sector.y*2 + 1;
		}
		else if ( port == SectorData::POS_X ) {
			g.x = (sector.x+1)*2;
			g.y = sector.y*2 + 1;
		}
		else if ( port == SectorData::NEG_Y ) {
			g.x = sector.x*2 + 1;
			g.y = sector.y*2;
		}
		else if ( port == SectorData::POS_Y ) {
			g.x = sector.x*2 + 1;
			g.y = (sector.y+1)*2;
		}
		else {
			GLASSERT( 0 );
		}
	}
	return g;
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


const SectorData& WorldInfo::GetSectorInfo( float fx, float fy, int *port ) const
{
	const SectorData& sd = GetSector( (int)fx, (int)fy );
	Vector2I v = { sd.x/SECTOR_SIZE, sd.y/SECTOR_SIZE };

	int axis = 0;
	if ( port ) {
		int dx = (int)fx - (v.x*SECTOR_SIZE + SECTOR_SIZE/2);
		int dy = (int)fy - (v.y*SECTOR_SIZE + SECTOR_SIZE/2);
		if ( abs(dx) > abs(dy) ) {
			*port = ( dx > 0 ) ? SectorData::POS_X : SectorData::NEG_X;
		}
		else {
			*port = ( dy > 0 ) ? SectorData::POS_Y : SectorData::NEG_Y;
		}
	}
	return sd;
}


float WorldInfo::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	GridEdge start = FromState( stateStart );	
	GridEdge end   = FromState( stateEnd );

	GLASSERT( HasGridEdge( start ));
	GLASSERT( HasGridEdge( end ));

	int len = abs( start.x - end.x ) + abs( start.y - end.y );
	return (float)len;
}


void  WorldInfo::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	adjacent->clear();
	const GridEdge g = FromState( state );
	GLASSERT( HasGridEdge( g ));

	static const GridEdge alt[] = {
		{ g.x + 1, g.y },
		{ g.x - 1, g.y },
		{ g.x, g.y + 1 },
		{ g.x, g.y - 1 } };

	for( int i=0; i<4; ++i ) {
		if ( HasGridEdge( alt[i] )) {
			micropather::StateCost sc = { ToState( alt[i] ), 0.5f };
			adjacent->push_back( sc );
		}
	}
}


void  WorldInfo::PrintStateInfo( void* state )
{
	GridEdge v = FromState( state );
	GLOUTPUT(( "%d,%d ", v.x, v.y ));
}

