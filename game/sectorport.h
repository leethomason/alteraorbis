#ifndef SECTORPORT_INCLUDED
#define SECTORPORT_INCLUDED

#include "../grinliz/glvector.h"

// structure to bind together a location on the grid
struct SectorPort
{
	SectorPort() { sector.Zero(); port=0; }
	SectorPort( const SectorPort& sp ) {
		sector = sp.sector;
		port   = sp.port;
	}

	bool IsValid() const { return sector.x > 0 && port; } 
	void Zero() { sector.Zero(); port = 0; }

	grinliz::Vector2I	sector;
	int					port;
};


#endif // SECTORPORT_INCLUDED