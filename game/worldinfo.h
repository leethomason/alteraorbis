#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "../xarchive/glstreamer.h"
#include "../tinyxml2/tinyxml2.h"
#include "../micropather/micropather.h"
#include "gamelimits.h"
#include "../engine/enginelimits.h"


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


// Passed to the pather as a 32 bit value (not a pointer.)
// The GridEdge is always "round down", so there is only the "neg-x" and "neg-y" flavor.
struct GridEdge
{
	enum {
		HORIZONTAL,
		VERTICAL
	};
	U8 alignment;
	S8 x;	// *sector* coordinates, not grid.
	S16 y;	// could be U8, but don't want undefined bits.

	bool operator==( const GridEdge& rhs ) const { return ( x == rhs.x && y == rhs.y && alignment == rhs.alignment ); }

	grinliz::Vector2F GridCenter() const {
		grinliz::Vector2F c = { 0, 0 };
		if ( alignment == HORIZONTAL ) {
			c.Set( (float)(x*SECTOR_SIZE + SECTOR_SIZE/2), (float)(y*SECTOR_SIZE) );
		}
		else {
			c.Set( (float)(x*SECTOR_SIZE), (float)(y*SECTOR_SIZE + SECTOR_SIZE/2));
		}
		return c;
	}

	bool IsValid() const { return x > 0 || y > 0; }
};


class WorldInfo : public micropather::Graph
{
public:
	SectorData sectorData[NUM_SECTORS*NUM_SECTORS];

	WorldInfo();
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

	// Get the grid edge from the sector and the axis.
	GridEdge GetGridEdge( const grinliz::Vector2I& sector, int axis ) const;

	// Get the current gridedge from the grid coordinates.
	// Will be !Valid() if there is an error, or if the pos isn't on the grid.
	GridEdge GetGridEdge( float x, float y ) const;

	// Get the cx, cy of the sector from an arbitrary coordinate.
	// 'axis' is optional, and returns NEG_X, etc.
	// GridEdge is optional, and returns the gridEdge at that axis, if it exists.
	const SectorData& GetSectorInfo( float x, float y, int* axis=0, GridEdge* edge=0 ) const;

	int NearestPort( const grinliz::Vector2I& sector, const grinliz::Vector2F& p ) const;

private:
	bool HasGridEdge( GridEdge g ) const;

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
};

#endif // LUMOS_WORLDINFO_INCLUDED