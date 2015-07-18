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


int WorldInfo::Solve( GridBlock start, GridBlock end, grinliz::CDynArray<GridBlock>* path )
{
	GLASSERT(!start.IsZero());
	GLASSERT(!end.IsZero());
	GLASSERT(worldGrid[INDEX(start)].IsGrid());
	GLASSERT(worldGrid[INDEX(end)].IsGrid());

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


GridBlock WorldInfo::GetGridBlock( const grinliz::Vector2I& sector, int port ) const
{
	GLASSERT( sector.x >= 0 && sector.x < NUM_SECTORS && sector.y >= 0 && sector.y < NUM_SECTORS );
	const SectorData& sd = sectorData[sector.y*NUM_SECTORS+sector.x];
	GridBlock g = { 0, 0 };
	static const int HALF_SIZE = SECTOR_SIZE / 2;

	if ( sd.ports & port ) {
		if ( port == SectorData::NEG_X ) {
			g.x = sector.x * SECTOR_SIZE;
			g.y = sector.y * SECTOR_SIZE + HALF_SIZE;
		}
		else if ( port == SectorData::POS_X ) {
			g.x = (sector.x+1)*SECTOR_SIZE;
			g.y = sector.y * SECTOR_SIZE + HALF_SIZE;
		}
		else if ( port == SectorData::NEG_Y ) {
			g.x = sector.x * SECTOR_SIZE + HALF_SIZE;
			g.y = sector.y * SECTOR_SIZE;
		}
		else if ( port == SectorData::POS_Y ) {
			g.x = sector.x * SECTOR_SIZE + HALF_SIZE;
			g.y = (sector.y+1)*SECTOR_SIZE;
		}
		else {
			GLASSERT( 0 );
		}
	}
	return g;
}

float WorldInfo::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	GridBlock start = FromState( stateStart );	
	GridBlock end   = FromState( stateEnd );

	GLASSERT(worldGrid[INDEX(start)].IsGrid());
	GLASSERT(worldGrid[INDEX(end)].IsGrid());

	int len = abs( start.x - end.x ) + abs( start.y - end.y );
	return (float)len;
}


void  WorldInfo::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	adjacent->clear();
	const GridBlock g = FromState( state );
	GLASSERT(worldGrid[INDEX(g)].IsGrid());

	// If we are not at a Port or Corner, then the next step is
	// to the 2 near Ports/Corners. That's a one way move.
	// If we are Port or Corner, connect to the next Corner or Port.

	static const int HALF = SECTOR_SIZE / 2;
	Rectangle2I bounds;
	bounds.Set(0, 0, mapWidth - 1, mapHeight - 1);
	CArray<Vector2I, 4> adj;

	int modX = g.x % SECTOR_SIZE;
	int modY = g.y % SECTOR_SIZE;
	Vector2I sector = { g.x / SECTOR_SIZE, g.y / SECTOR_SIZE };

	if (modX == 0 && modY == 0) {
		// Corner.
		adj.PushArr(4);
		adj[0].Set(g.x + HALF, g.y);
		adj[1].Set(g.x, g.y + HALF);
		adj[2].Set(g.x - HALF, g.y);
		adj[3].Set(g.x, g.y - HALF);
	}
	else if (modX && modY == 0) {
		// Port (east-west)
		adj.PushArr(2);
		adj[0].Set(sector.x * SECTOR_SIZE, g.y);
		adj[1].Set((sector.x + 1) * SECTOR_SIZE, g.y);
	}
	else {		//if (modX == 0 && modY) {
		// Port (north-south)
		adj.PushArr(2);
		adj[0].Set(g.x, sector.y * SECTOR_SIZE);
		adj[1].Set(g.x, (sector.y + 1) * SECTOR_SIZE);
	}

	for (int i = 0; i < adj.Size(); ++i) {
		Vector2I v = { adj[i].x, adj[i].y };
		GridBlock vgb = { S16(adj[i].x), S16(adj[i].y) };
		if (bounds.Contains(v) && worldGrid[INDEX(vgb)].IsGrid()) {
			micropather::StateCost sc = { ToState(vgb), HALF };
			adjacent->push_back(sc);
		}
	}
}


void  WorldInfo::PrintStateInfo( void* state )
{
	GridBlock v = FromState( state );
	GLOUTPUT(( "%d,%d ", v.x, v.y ));
	(void)v;
}

