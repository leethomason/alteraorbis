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
	static grinliz::Vector2I	SectorID( float x, float y );
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


class WorldInfo : public micropather::Graph
{
public:
	SectorData sectorData[NUM_SECTORS*NUM_SECTORS];

	void Serialize( XStream* );
	void Save( tinyxml2::XMLPrinter* printer );
	void Load( const tinyxml2::XMLElement& parent );

	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

private:
	grinliz::Vector2<S16> FromState( void* state ) {
		grinliz::Vector2<S16> v = *((grinliz::Vector2<S16>*)state);
		return v;
	}
	void* ToState( grinliz::Vector2<S16> v ) {
		void* r = (void*)(*((U32*)&v));
		return r;
	}
};

#endif // LUMOS_WORLDINFO_INCLUDED