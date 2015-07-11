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

#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "gamelimits.h"
#include "lumosmath.h"
#include "../xarchive/glstreamer.h"
#include "../tinyxml2/tinyxml2.h"
#include "../micropather/micropather.h"
#include "../engine/enginelimits.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrectangle.h"

struct WorldGrid;
namespace micropather {
	class MicroPather;
}

class SectorData
{
public:
	SectorData() : ports(0), area(0), pather(0) {
		core.Zero();
		sector.Zero();
	}
	~SectorData();

	void Serialize( XStream* xs );

	enum { 
		NEG_X	= 1, 
		POS_X	= 2, 
		NEG_Y	= 4,		
		POS_Y	= 8 
	};
	grinliz::Vector2I			sector;
	int							ports;		// if attached to the grid, has ports. 
	grinliz::Vector2I			core;		// core location, in map coordinates
	int							area;
	micropather::MicroPather*	pather;
	grinliz::IString			name;

	bool HasCore() const				{ return core.x > 0 && core.y > 0; }
	grinliz::Vector2I CoreLoc() const	{ return SectorBounds(sector).Center(); }

	// Nearest port to 'pos'. There are no limits on pos.
	int NearestPort( const grinliz::Vector2I& pos ) const;	
	int NearestPort( const grinliz::Vector2F pos ) const {
		grinliz::Vector2I p = { int(grinliz::LRintf(pos.x)), int(grinliz::LRintf(pos.y)) };
		return NearestPort( p );
	}
	int RandomPort(grinliz::Random* random) const;

	grinliz::Rectangle2I GetPortLoc( int port ) const;
	grinliz::Rectangle2I Bounds() const;
	grinliz::Rectangle2I InnerBounds() const;

	// ------- Utility -------- //
	static grinliz::Vector2F PortPos( const grinliz::Rectangle2I portBounds, U32 seed );
};


typedef grinliz::Vector2<S16> GridBlock;

inline GridBlock MapToGridBlock(float x, float y)
{
	// x: 32,32 is the gridblock. [31.0, 33.0)->32
	int gbx = int(x + 1.0) & (~1);
	int gby = int(y + 1.0) & (~1);
	GridBlock gb = {  S16(gbx), S16(gby) };
	return gb;
}

class WorldInfo : public micropather::Graph
{
public:
	WorldInfo( const WorldGrid* worldGrid, int mapWidth, int mapHeight );
	~WorldInfo();
	
	void Serialize( XStream* );
	void Save( tinyxml2::XMLPrinter* printer );
	void Load( const tinyxml2::XMLElement& parent );

	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	int Solve( GridBlock start, GridBlock end, grinliz::CDynArray<GridBlock>* path );

	// Get the current sector from the grid coordinates.
	const SectorData& GetSector( const grinliz::Vector2I& sector ) const {
		GLASSERT( sector.x >= 0 && sector.y >= 0 && sector.x < NUM_SECTORS && sector.y < NUM_SECTORS );
		return sectorData[NUM_SECTORS*sector.y+sector.x];
	}

	// Get the grid edge from the sector and the port.
	// Return 0,0 if it doesn't exist.
	GridBlock GetGridBlock( const grinliz::Vector2I& sector, int port ) const;

	// Get the cx, cy of the sector from an arbitrary coordinate.
	const SectorData& GetSectorInfo( float x, float y ) const;

	const SectorData* SectorDataMem() const { return sectorData; }
	SectorData* SectorDataMemMutable()		{ return sectorData; }

private:
	int INDEX(int x, int y) const { return y*mapWidth + x; }
	int INDEX(const GridBlock& gb) const { return gb.y*mapWidth + gb.x; }

	GridBlock FromState( void* state ) {
		intptr_t iState = reinterpret_cast<intptr_t >(state);
		GridBlock v = { S16(iState & 0xffff), S16((iState >> 16) & 0xffff) };
		return v;
	}
	void* ToState( GridBlock v ) {
		intptr_t iState = v.x | (v.y << 16);
		void* r = reinterpret_cast<void*>(iState);
		return r;
	}
	micropather::MPVector< void* > patherVector;
	micropather::MicroPather* pather;

	int mapWidth;
	int mapHeight;
	const WorldGrid* worldGrid;

	SectorData sectorData[NUM_SECTORS*NUM_SECTORS];
};


#endif // LUMOS_WORLDINFO_INCLUDED