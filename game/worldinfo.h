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

#include "../xarchive/glstreamer.h"
#include "../tinyxml2/tinyxml2.h"
#include "../micropather/micropather.h"
#include "gamelimits.h"
#include "../engine/enginelimits.h"
#include "../grinliz/glutil.h"

struct WorldGrid;
namespace micropather {
	class MicroPather;
}

class SectorData
{
public:
	SectorData() : x(0), y(0), ports(0), area(0), pather(0) {
		core.Zero();
	}
	~SectorData();

	void Serialize( XStream* xs );

	enum { 
		NEG_X	=1, 
		POS_X	=2, 
		NEG_Y	=4,		
		POS_Y	=8 
	};
	int							x, y;		// grid position (not sector position)
	int							ports;		// if attached to the grid, has ports. 
	grinliz::Vector2I			core;		// core location, in map coordinates
	int							area;
	micropather::MicroPather*	pather;
	grinliz::IString			name;

	bool HasCore() const	{ return core.x > 0 && core.y > 0; }

	// Nearest port to 'pos'. There are no limits on pos.
	int NearestPort( const grinliz::Vector2I& pos ) const;	
	int NearestPort( const grinliz::Vector2F pos ) const {
		grinliz::Vector2I p = { grinliz::LRintf(pos.x), grinliz::LRintf(pos.y) };
		return NearestPort( p );
	}
	grinliz::Rectangle2I GetPortLoc( int port ) const;
	grinliz::Rectangle2I Bounds() const;
	grinliz::Rectangle2I InnerBounds() const;

	// ------- Utility -------- //
	static grinliz::Vector2I SectorID( float x, float y ) {
		grinliz::Vector2I v = { (int)x/SECTOR_SIZE, (int)y/SECTOR_SIZE };
		return v;
	}
	// Get the bounds of the sector from an arbitrary coordinate
	static grinliz::Rectangle2I	SectorBounds( float x, float y );
	static grinliz::Rectangle3F	SectorBounds3( float x, float y ) {
		grinliz::Rectangle2I r = SectorBounds( x, y );
		grinliz::Rectangle3F r3;
		r3.Set( (float)r.min.x, 0, (float)r.min.y, (float)r.max.x, MAP_HEIGHT, (float)r.max.y );
		return r3;
	}
	static grinliz::Rectangle2I InnerSectorBounds( float x, float y ) {
		grinliz::Rectangle2I r = SectorBounds( x, y );
		r.Outset( -1 );
		return r;
	}
	static grinliz::Vector2F PortPos( const grinliz::Rectangle2I portBounds, U32 seed );
};

/*
	Double the sector coordinates.
	2 3 4
	+---+
	|1,1|
	|	|
	+---+

*/
typedef grinliz::Vector2<S16> GridEdge;

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

	int Solve( GridEdge start, GridEdge end, grinliz::CDynArray<GridEdge>* path );

	// Get the current sector from the grid coordinates.
	const SectorData& GetSector( const grinliz::Vector2I& sector ) const {
		GLASSERT( sector.x >= 0 && sector.y >= 0 && sector.x < NUM_SECTORS && sector.y < NUM_SECTORS );
		return sectorData[NUM_SECTORS*sector.y+sector.x];
	}

	// Get the grid edge from the sector and the port.
	// Return 0,0 if it doesn't exist.
	GridEdge GetGridEdge( const grinliz::Vector2I& sector, int port ) const;

	// Get the cx, cy of the sector from an arbitrary coordinate.
	const SectorData& GetSectorInfo( float x, float y ) const;

	grinliz::Vector2I GridEdgeToSector( GridEdge e ) const {
		grinliz::Vector2I s = { e.x/2, e.y/2 };
		GLASSERT( s.x >= 0 && s.x < NUM_SECTORS && s.y >=0 && s.y < NUM_SECTORS );
		return s;
	}

	grinliz::Vector2I GridEdgeToMap( GridEdge e ) const {
		grinliz::Vector2I m = { e.x * SECTOR_SIZE / 2, e.y * SECTOR_SIZE / 2 };
		GLASSERT( m.x >= 0 && m.x < MAX_MAP_SIZE && m.y >= 0 && m.y < MAX_MAP_SIZE );
		return m;
	}

	grinliz::Vector2F GridEdgeToMapF( GridEdge e ) const {
		grinliz::Vector2I m = GridEdgeToMap( e );
		grinliz::Vector2F v = { (float)m.x, (float)m.y };
		return v;
	}

	GridEdge MapToGridEdge( int x, int y ) const {
		GridEdge ge;

		ge.x = 2*x / SECTOR_SIZE;
		ge.y = 2*y / SECTOR_SIZE;

		return ge;
	}

	bool HasGridEdge( const GridEdge& ge ) const {
		return HasGridEdge( ge.x, ge.y );
	}
	bool HasGridEdge( int geX, int geY ) const;

	const SectorData* SectorDataMem() const { return sectorData; }
	SectorData* SectorDataMemMutable()		{ return sectorData; }

private:

	GridEdge FromState( void* state ) {
		GLASSERT( sizeof(GridEdge) == sizeof(void*) );
		GridEdge v = *((GridEdge*)&state);
		return v;
	}
	void* ToState( GridEdge v ) {
		void* r = (void*)(*((U32*)&v));
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