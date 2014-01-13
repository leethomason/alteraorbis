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

#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcolor.h"

static const int MAX_ROCK_HEIGHT		= 3;
static const int HP_PER_HEIGHT			= 150;	// 511 is the max value, 1/3 of this
static const int POOL_HEIGHT			= 2;

struct WorldGrid {

public:
	unsigned extBlock			: 1;	// used by the map. prevents allocating another structure.
private:
	// memset(0) should work, and make it water.

	unsigned isLand				: 1;
	unsigned isGrid				: 1;
	unsigned isPort				: 1;
	unsigned pave				: 2;	// 0-3, the pave style, if >0
	unsigned isPorch			: 1;	// only used for rendering

	unsigned nominalRockHeight	: 2;	// 0-3
	unsigned rockType			: 1;	// ROCK, ICE
	unsigned magma				: 1;	// land, rock, or water can be set to magma
	unsigned rockHeight			: 2;
	unsigned hasPool			: 1;

	unsigned zoneSize			: 5;	// 0-31 (need 0-16)
	unsigned hp					: 9;	// 0-511

	unsigned debugAdjacent		: 1;
	unsigned debugPath			: 1;
	unsigned debugOrigin		: 1;

	unsigned path				: 10;	// 2 bits each: core, port0-3

public:
	bool IsBlocked() const			{ return extBlock || (!isLand) || isGrid || rockHeight || hasPool; }
	bool IsPassable() const			{ return !IsBlocked(); }
	
	// does this and rhs render the same voxel?
	int VoxelEqual( const WorldGrid& wg ) const {
		return	isLand == wg.isLand &&
				isGrid == wg.isGrid &&
				isPort == wg.isPort &&
				pave   == wg.pave &&
				isPorch == wg.isPorch &&
				rockHeight == wg.rockHeight &&
				magma == wg.magma &&
				hasPool == wg.hasPool;
	}

	enum {
		ROCK,
		ICE
	};

	grinliz::Color4U8 ToColor() const {
		grinliz::Color4U8 c = { 0, 0, 0, 255 };

		if ( isGrid ) {
			c.Set( 120, 180, 180, 255 );
		}
		else if ( isPort ) {
			c.Set( 180, 180, 0, 255 );
		}
		else if ( isLand ) {
			if ( nominalRockHeight == 0 ) {
				c.Set( 0, 140, 0, 255 );
			}
			else {
				int add = nominalRockHeight*40;
				c.Set( add, 100+add, add, 255 );
			}
		}
		else {
			c.Set( 0, 0, 200, 255 );
		}
		return c;
	}

	bool IsLand() const			{ return isLand != 0; }
	bool IsPort() const			{ return isPort != 0; }
	bool IsGrid() const			{ return isGrid != 0; }
	bool IsPorch() const		{ return isPorch != 0; }

	enum {
		WATER,
		GRID,
		PORT,
		LAND,
		NUM_LAYERS,
		NUM_PAVE = 4	// including 0, which is "no pavement"
	};
	int Layer() const			{ 
		int layer = WATER;
		if ( isGrid ) layer = GRID;
		else if ( isPort ) layer = PORT;
		else if ( isLand ) layer = LAND;
		return layer;
	}
	int Pave() const { return pave; }
	void SetPave( int p ) {
		GLASSERT( p >=0 && p < NUM_PAVE );
		GLASSERT( Layer() == LAND );
		GLASSERT( RockHeight() == 0 );
		pave = p;
	}
	void SetPorch( bool on ) {
		isPorch = on ? 1 : 0;
	}

	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }

	void SetGrid()				{ isLand = 1; isGrid = 1; }
	void SetPort()				{ isLand = 1; isPort = 1; }
	void SetLandAndRock( U8 h )	{
		// So confusing. Max rock height=3, but land goes from 1-4 to be distinct from water.
		// Subtract here.
		if ( !h ) {
			SetWater();	
		}
		else {
			SetLand();
			SetNominalRockHeight( h-1 );
		}
	}

	void SetLand()				{ 
		GLASSERT( sizeof(WorldGrid) == sizeof(U32)*2 );	// WorldGrid can be any size; just make sure it is the one intended 
		isLand = 1;
	}

	int NominalRockHeight() const { return nominalRockHeight; }
	void SetNominalRockHeight( int h ) {
		GLASSERT( IsLand() );
		GLASSERT( h >= 0 && h <= MAX_ROCK_HEIGHT );
		nominalRockHeight = h;
	}

	int RockHeight() const { return rockHeight; }
	void SetRockHeight( int h ) {
		GLASSERT( IsLand() || (h==0) );
		GLASSERT( h >= 0 && h <= MAX_ROCK_HEIGHT );
		rockHeight = h;
		if ( h > 0 ) pave = 0;	// rock clears out pavement.
	}

	int RockType() const { return rockType; }
	void SetRockType( int type ) {
		GLASSERT( type >=0 && type <= ICE );
		rockType = type;
	}

	int Height() const {
		if ( Pool() ) return POOL_HEIGHT;
		return RockHeight();
	}

	bool Pool() const { return hasPool ? true : false; }
	void SetPool( bool p ) {
		GLASSERT( IsLand() || (p==false) );
		GLASSERT( !p || POOL_HEIGHT > (int)rockHeight );
		hasPool = p ? 1 : 0;
	}

	bool IsWater() const		{ return !IsLand(); }
	void SetWater()				{ 
		isLand = 0;
		nominalRockHeight = 0;
	}

	bool Magma() const			{ return magma != 0; }
	void SetMagma( bool m )		{ magma = m ? 1 : 0; if ( magma ) pave = 0; }

	int TotalHP() const			{ return rockHeight * HP_PER_HEIGHT; }
	int HP() const				{ return hp; }
	void DeltaHP( int delta ) {
		int points = hp;
		points += delta;
		if ( points < 0 ) points = 0;
		if ( points > TotalHP() ) points = TotalHP();
		hp = points;
		GLASSERT( hp == points );
	}

	bool DebugAdjacent() const		{ return debugAdjacent != 0; }
	void SetDebugAdjacent( bool v ) { debugAdjacent = v ? 1 : 0; }

	bool DebugOrigin() const		{ return debugOrigin != 0; }
	void SetDebugOrigin( bool v )	{ debugOrigin = v ? 1 : 0; }

	bool DebugPath() const			{ return debugPath != 0; }
	void SetDebugPath( bool v )		{ debugPath = v ? 1 : 0; }

	U32 ZoneSize() const			{ return zoneSize;}

	// x & y must be in zone.
	grinliz::Vector2I ZoneOrigin( int x, int y ) const {
		U32 mask = 0xffffffff;
		if ( zoneSize ) {
			mask = ~(zoneSize-1);
		}
		grinliz::Vector2I v = { x & mask, y & mask };
		return v;
	}

	void SetZoneSize( int s )	{ 
		zoneSize = s;
		GLASSERT( s == zoneSize );
	}

	void SetPath( int pathData )	{ path = pathData; }

	enum { CORE, PORT_POS_X, PORT_POS_Y, PORT_NEG_X, PORT_NEG_Y, NUM_DEST };
	const grinliz::Vector2I& Path( int dest ) const {
		static const grinliz::Vector2I DIR[4] = {
			{1,0}, {0,1}, {-1,0}, {0,-1}
		};
		int index = (path >> (dest*2)) & 3;
		return DIR[index];
	}

};

#endif // LUMOS_WORLD_GRID_INCLUDED
