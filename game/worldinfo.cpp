#include "worldinfo.h"

#include "../grinliz/glrandom.h"

using namespace tinyxml2;
using namespace grinliz;

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
	Vector2<S16> start = FromState( stateStart );	
	Vector2<S16> end   = FromState( stateEnd );
	float len = (float)abs(start.x - end.x) + (float)abs(start.y - end.y);
	return len;
}


void  WorldInfo::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	adjacent->clear();
	const Vector2<S16> v = FromState( state );
	GLASSERT( InRange( (int)v.x, 0, (int)NUM_SECTORS-1 ));
	GLASSERT( InRange( (int)v.y, 0, (int)NUM_SECTORS-1 ));

	const SectorData& sd = sectorData[v.y+NUM_SECTORS+v.x];

	static const Vector2<S16> delta[4] = { {-1,0}, {1,0}, {0,-1}, {0,1} };

	// Interpret the grid point in the direction of the origin.
	for( int i=0; i<4; ++i ) {
		int port = 1<<i;
		if ( sd.ports & port ) {
			Vector2<S16> alt = v + delta[i];
			micropather::StateCost sc = { ToState(alt), 1 };
			adjacent->push_back( sc );
		}
	}
}


void  WorldInfo::PrintStateInfo( void* state )
{
	Vector2<S16> v = FromState( state );
	GLOUTPUT(( "%d,%d ", v.x, v.y ));
}

