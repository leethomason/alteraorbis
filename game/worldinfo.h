#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "../script/worldgen.h"
#include "../xarchive/glstreamer.h"
#include "gamelimits.h"

class WorldInfo
{
public:
	SectorData sectorData[NUM_SECTORS*NUM_SECTORS];

	void Serialize( XStream* );
	void Save( tinyxml2::XMLPrinter* printer );
	void Load( const tinyxml2::XMLElement& parent );

private:
	
};

#endif // LUMOS_WORLDINFO_INCLUDED