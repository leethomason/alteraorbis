#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "../xarchive/glstreamer.h"
#include "../tinyxml2/tinyxml2.h"
#include "../micropather/micropather.h"
#include "gamelimits.h"
#include "../engine/enginelimits.h"

struct WorldGrid;

class SectorData
{
public:
	SectorData() : x(0), y(0), ports(0), area(0) {
		core.Zero();
	}

	void Serialize( XStream* xs );

	enum { 
		NEG_X	=1, 
		POS_X	=2, 
		NEG_Y	=4,		
		POS_Y	=8 
	};
	int  x, y;		// grid position (not sector position)
	int  ports;		// if attached to the grid, has ports. 
	grinliz::Vector2I core;
	int  area;

	bool HasCore() const { return core.x > 0 && core.y > 0; }
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
	SectorData sectorData[NUM_SECTORS*NUM_SECTORS];

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
	const SectorData& GetSector( int x, int y ) const {
		int sx = x / SECTOR_SIZE;
		int sy = y / SECTOR_SIZE;
		GLASSERT( sx >= 0 && sy >= 0 && sx < NUM_SECTORS && sy < NUM_SECTORS );
		return sectorData[NUM_SECTORS*sy+sx];
	}

	// Get the grid edge from the sector and the port.
	// Return 0,0 if it doesn't exist.
	GridEdge GetGridEdge( const grinliz::Vector2I& sector, int port ) const;

	// Get the cx, cy of the sector from an arbitrary coordinate.
	// 'nearestPort' is optional.
	const SectorData& GetSectorInfo( float x, float y, int* nearestPort ) const;

	int NearestPort( const grinliz::Vector2I& sector, const grinliz::Vector2F& p ) const;

	grinliz::Vector2I GridEdgeToSector( int x, int y ) const {
		grinliz::Vector2I s = { x/2, y/2 };
		GLASSERT( x >= 0 && x < NUM_SECTORS && y >=0 && y < NUM_SECTORS );
		return s;
	}
	grinliz::Vector2I GridEdgeToMap( int x, int y ) const {
		grinliz::Vector2I m = { x * SECTOR_SIZE / 2, y * SECTOR_SIZE / 2 };
		GLASSERT( x >= 0 && x < MAX_MAP_SIZE && y >= 0 && y < MAX_MAP_SIZE );
		return m;
	}

	GridEdge MapToGridEdge( int x, int y ) const {
		GridEdge ge = { x*2 / SECTOR_SIZE, y*2 / SECTOR_SIZE };
		return ge;
	}

	bool HasGridEdge( const GridEdge& ge ) const {
		return HasGridEdge( ge.x, ge.y );
	}
	bool HasGridEdge( int geX, int geY ) const;

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
};

#endif // LUMOS_WORLDINFO_INCLUDED