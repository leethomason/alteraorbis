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

float WorldInfo::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	GridEdge start = FromState( stateStart );	
	GridEdge end   = FromState( stateEnd );

	Vector2F cStart = start.Center();
	Vector2F cEnd   = end.Center();

	float len = fabs( cStart.x - cEnd.x ) + fabs( cStart.y - cEnd.y );
	return len;
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


void  WorldInfo::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	adjacent->clear();
	const GridEdge g = FromState( state );
	GLASSERT( HasGridEdge( g ));

	/*
			1,1  | V2,0
		+-------+--- H2,1
				| V2,1

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

