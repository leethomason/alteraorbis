/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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