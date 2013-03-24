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

