#ifndef LUMOS_WORLDGEN_INCLUDED
#define LUMOS_WORLDGEN_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"

#include "../xegame/xegamelimits.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"
#include "../engine/enginelimits.h"

namespace grinliz {
class PerlinNoise;
};


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


class WorldGen
{
public:
	WorldGen();
	~WorldGen();

	// Done in order:
	//  - Start()
	//  - Do() for each Y
	//  - End()
	//  - optional: WriteMarker()
	void StartLandAndWater( U32 seed0, U32 seed1 );
	void DoLandAndWater( int y );
	bool EndLandAndWater( float fractionLand );
	void WriteMarker();

	enum {
		WATER,
		LAND0,
		LAND1,
		LAND2,
		LAND3,
		GRID,
		PORT,
		CORE,
		NUM_TYPES
	};

	void SetHeight( int x, int y, int h ) {
		int index = y*SIZE+x;
		GLASSERT( x >= 0 && x < SIZE && y >= 0 && y < SIZE );
		GLASSERT( h >= WATER && h < NUM_TYPES );
		land[index] = h;
	}

	void CutRoads( U32 seed, SectorData* data );
	void ProcessSectors( U32 seed, SectorData* data );
	void GenerateTerrain( U32 seed, SectorData* data );

	const U8* Land() const						{ return land; }

	enum {
		SIZE			= MAX_MAP_SIZE
	};

private:
	int  CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh );
	void Draw( const grinliz::Rectangle2I& r, int land );
	int  CalcSectorArea( int x, int y );
	void AddPorts( SectorData* sector );
	int CalcSectorAreaFromFill( SectorData* s, const grinliz::Vector2I& origin, bool* allPortsColored );
	void DepositLand( SectorData* s, U32 seed, int n );
	void Filter( const grinliz::Rectangle2I& bounds );

	float* flixels;
	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	U8* land;
	U8* color;
};

#endif // LUMOS_WORLDGEN_INCLUDED