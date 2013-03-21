#ifndef LUMOS_WORLDGEN_INCLUDED
#define LUMOS_WORLDGEN_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"

#include "../xegame/xegamelimits.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"

namespace grinliz {
class PerlinNoise;
};

struct WorldFeature {
	int  id;						// unique id
	bool land;						// true: continent or island, false: ocean
	grinliz::Rectangle2I bounds;	// bounds
	int area;						// actual area

	void Save( tinyxml2::XMLPrinter* );
	void Serialize( XStream* );
	void Load( const tinyxml2::XMLElement& element );

	class Compare
	{
	public:
		static bool Less( const WorldFeature& s0, const WorldFeature& s1 ) { return s0.area > s1.area; }
	};
};


class SectorData
{
public:
	SectorData() : x(0), y(0), slots(0), hasCore(false), area(0) {}
	enum { 
		NEG_X	=1, 
		POS_X	=2, 
		NEG_Y	=4,		
		POS_Y	=8 
	};
	int  x, y;		// grid position (not sector position)
	int  slots;		// if attached to the grid, has slots. 
	bool hasCore;
	int  area;
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

	void ApplyHeight( int x, int y, int h, int blend ) {
		int index = y*SIZE+x;
		GLASSERT( blend >= 0 && blend <= 256 );
		if ( land[index] ) {
			if ( blend >= 0 ) {
				land[index] = ((land[index] * (256-blend)) + (h * blend)) >> 8;
			}
			else {
				if ( land[index] ) 
					land[index] = h;
			}
			if ( land[index] == 0 )
				land[index] = 1;
		}
	}

	void CutRoads( U32 seed, SectorData* data );
	void ProcessSectors( U32 seed, SectorData* data );

	enum {
		WATER,
		LAND0,
		LAND1,
		LAND2,
		LAND3,
		GRID,
		PORT
	};

	const U8* Land() const						{ return land; }

	enum {
		SIZE			= MAX_MAP_SIZE,
		REGION_SIZE		= 64,
		NUM_REGIONS		= SIZE / REGION_SIZE
	};

private:
	int  CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh );
	void Draw( const grinliz::Rectangle2I& r, int land );
	int  CalcSectorArea( int x, int y );
	void AddSlots( SectorData* sector );

	float* flixels;
	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	U8* land;
};

#endif // LUMOS_WORLDGEN_INCLUDED