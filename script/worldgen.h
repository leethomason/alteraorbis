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

#include "../xegame/xegamelimits.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"
#include "../engine/enginelimits.h"
#include "../game/worldinfo.h"
#include "../micropather/micropather.h"

namespace grinliz {
class PerlinNoise;
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
	// Connect up portal or core.
	void GenerateTerrain( U32 seed, SectorData* data );

	const U8* Land() const						{ return land; }

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

	/*
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state ) { GLASSERT( 0 ); }
	*/

private:
	int  CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh );
	void Draw( const grinliz::Rectangle2I& r, int land );
	int  CalcSectorArea( int x, int y );
	void AddPorts( SectorData* sector );
	int CalcSectorAreaFromFill( SectorData* s, const grinliz::Vector2I& origin, bool* allPortsColored );
	void DepositLand( SectorData* s, U32 seed, int n );
	void Filter( const grinliz::Rectangle2I& bounds );
	void RemoveUncoloredLand( SectorData* s );

	float* flixels;
	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	U8* land;
	U8* color;
};

#endif // LUMOS_WORLDGEN_INCLUDED