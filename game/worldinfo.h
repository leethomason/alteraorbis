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
	// Get the cx, cy of the sector from an arbitrary coordinate.
	// 'axis' is optional, and returns NEG_X, etc.
	static grinliz::Vector2I	SectorID( float x, float y, int* axis=0 );
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
struct GridEdge
{
	enum {
		HORIZONTAL,
		VERTICAL
	};
	U8 alignment;
	S8 x;
	S16 y;	// could be U8, but don't want undefined bits.

	grinliz::Vector2F Center() const {
		grinliz::Vector2F c = { 0, 0 };
		if ( alignment == HORIZONTAL ) {
			c.Set( (float)x + 0.5f, (float)y );
		}
		else {
			c.Set( (float)x, (float)y + 0.5f );
		}
		return c;
	}
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

	bool HasGridEdge( GridEdge g ) const;
	const SectorData& GetSector( int x, int y ) const {
		grinliz::Vector2I v = SectorData::SectorID( (float)x, (float)y );
		return sectorData[NUM_SECTORS*v.y+v.x];
	}

private:
	GridEdge FromState( void* state ) {
		GLASSERT( sizeof(GridEdge) == sizeof(void*) );
		GridEdge v = *((GridEdge*)state);
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