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

#ifndef LUMOS_WORLDGEN_INCLUDED
#define LUMOS_WORLDGEN_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glcolor.h"

#include "../xegame/xegamelimits.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"
#include "../engine/enginelimits.h"
#include "../game/worldinfo.h"

namespace grinliz {
class PerlinNoise;
};


class WorldGen
{
public:
	WorldGen();
	~WorldGen();

	void LoadFeatures( const char* path );

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
	// Connect up portal or core.
	void GenerateTerrain( U32 seed, SectorData* data );

	const U8*	Land() const						{ return land; }

	// Bits: 
	//	low 2:	Core
	//	next 2:	Port0
	//	next 2:	Port1
	//	next 2:	Port2
	//	next 2:	Port3
	enum {
		EAST, NORTH, WEST, SOUTH
	};
	const U16*	Path() const						{ return path; }

	enum {
		SIZE			= MAX_MAP_SIZE
	};

	grinliz::Vector2I FromState( void* s ) {
		int i = (int)s;
		int y = i / SIZE;
		int x = i - y*SIZE;
		grinliz::Vector2I v = { x, y };
		return v;
	}

	void* ToState( const grinliz::Vector2I& v ) {
		int i = v.y*SIZE + v.x;
		return (void*)i;
	}

private:
	int INDEX( int x, int y ) const					{ return y*SIZE + x; }
	int INDEX( const grinliz::Vector2I& v ) const	{ return v.y*SIZE + v.x; }

	int  CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh );
	void Draw( const grinliz::Rectangle2I& r, int land );
	int  CalcSectorArea( int x, int y );
	void AddPorts( SectorData* sector );
	void DepositLand( SectorData* s, U32 seed, int n );
	void Filter( const grinliz::Rectangle2I& bounds );
	void PlaceFeature( int feature, const grinliz::Vector2I& sector, int rotation );

	// returns area
	int Color( const grinliz::Rectangle2I& bounds, const grinliz::Vector2I& origin );
	int PortsColored( const SectorData* d );

	void RemoveUncoloredLand( SectorData* s );
	void CalcPath( const SectorData* s );

	float* flixels;
	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	U8*			land;
	U16*		color;		// used in 2 differnt ways: 1) to determine if land is connected to a port, and 2) to determine the path to cores and ports
	U16*		path;

	grinliz::Vector2I	featuresSize;
	grinliz::Color4U8*	features;	// used to pull in the PNG file of the features.
};

#endif // LUMOS_WORLDGEN_INCLUDED