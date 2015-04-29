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
static const int HP_PER_PLANT_STAGE		= 20;	// a square in there: 20 -> 320	
static const int FLUID_PER_ROCK			= 4;

/*	Fluid Blocking creates all kinds of problems: what happens when it goes over buildings? Cores? 
	May have to solve that, but for now, try to deal with it via MoveComponent and effects.
*/
//static const int FLUID_BLOCKING			= 2;

struct WorldGrid {
	friend class FluidSim;

public:
	// 1 bit
	unsigned extBlock			: 1;	// used by the map. prevents allocating another structure.

	enum {
		ROCK,
		ICE,
	};

	enum {
		WATER,	// can also be magma
		LAND,	// can still be pave, porch, circuit, or magma
		GRID,
		PORT,
		NUM_LAYERS
	};

	enum {
		FLUID_WATER = 0,
		FLUID_LAVA,
	};

private:
	// memset(0) should work, and make it water.

	// 1 + 13 = 14 bits
	unsigned land				: 2;	// WATER, LAND, GRID...
	unsigned rockType			: 1;	// ROCK, ICE
	unsigned pave				: 2;	// 0-3, the pave style, if >0
	unsigned isPorch			: 3;	// only used for rendering 0-6

	unsigned nominalRockHeight	: 2;	// 0-3
	unsigned magma				: 1;	// land, rock, or water can be set to magma
	unsigned rockHeight			: 2;

	// 14 + 27 = 41 bits
	unsigned zoneSize			: 5;	// 0-31 (need 0-16)
	unsigned hp					: 9;	// 0-511

	unsigned debugAdjacent		: 1;
	unsigned debugPath			: 1;
	unsigned debugOrigin		: 1;

	unsigned path				: 10;	// 2 bits each: core, port0-3

	// 41 + 12 = 53 bits
	unsigned plant				: 4;	// plant 1-8 (and potentially 1-15). also uses hp.
	unsigned stage				: 2;	// 0-3

	unsigned fluidEmitter		: 1;	// is emitter. type determined by 'fluidType'
	unsigned fluidHeight		: 4;	// 0-ROCK_HEIGHT * FLUID_PER_ROCK, 0-12
	unsigned fluidType			: 1;	// types of fluids: 0:water, 1:lava

	// 53 + 2 = 55 bits
	unsigned plantOnFire		: 1;	// NOTE: matches the array in the FluidSim
	unsigned plantOnShock		: 1;

public:
	bool IsBlocked() const			{ return    extBlock || (land == WATER) || (land == GRID) || rockHeight 
											 //|| (fluidHeight >= FLUID_BLOCKING) 
											 || (plant && stage >= 2); }
	bool IsPassable() const			{ return !IsBlocked(); }

	bool FluidSink() const			{ return (land == WATER) || (land == GRID) || (land == PORT); }
	
	// does this and rhs render the same voxel?
	int VoxelEqual( const WorldGrid& wg ) const {
		return	land == wg.land &&
			pave == wg.pave &&
			isPorch == wg.isPorch &&
			rockHeight == wg.rockHeight &&
			magma == wg.magma &&
			fluidEmitter == wg.fluidEmitter &&
			fluidHeight == wg.fluidHeight &&
			fluidType == wg.fluidType &&
			plantOnFire == wg.plantOnFire &&
			plantOnShock == wg.plantOnShock;
	}

	grinliz::Color4U8 ToColor() const {
		// Hardcoded palette values!
		grinliz::Color4U8 c = { 0, 0, 0, 255 };
		switch (land) {
		case WATER:		c.Set(0, 51, 178, 255);	break;
		case LAND:		c.Set(38, 98, 45, 255);		break;
		case GRID:		c.Set(16, 67, 34, 255); break;
		case PORT:		c.Set(232, 149, 0, 255); break;
		default: GLASSERT(0);
		}
		if ( land == LAND && nominalRockHeight) {
			if (nominalRockHeight == 1)
				c.Set( 126, 126, 126, 255 );
			else if (nominalRockHeight == 2)
				c.Set( 181, 181, 181, 255 );
			else
				c.Set( 235, 235, 235, 255 );
		}
		return c;
	}

	bool IsLand() const			{ return land >= LAND; }
	bool IsPort() const			{ return land == PORT; }
	bool IsGrid() const			{ return land == GRID; }

	enum {
		NUM_PAVE = 4,	// including 0, which is "no pavement"
		BASE_PORCH = 1,
		PORCH_UNREACHABLE = 7,
		NUM_PORCH = 8,	// 0: no porch, 1: basic porch, --,-,0,+,++,unreachable
	};
	int Land() const { return land; }

	int Pave() const { return pave; }
	void SetPave( int p ) {
		GLASSERT( p >=0 && p < NUM_PAVE );
		GLASSERT( Land() == LAND );
		GLASSERT( RockHeight() == 0 );
		pave = p;
	}

	int Porch() const		{ return isPorch; }
	void SetPorch(int porch) {
		// 0 no porch
		// 1 porch
		// 2 porch--
		// 3 porch-
		// 4 porch0
		// 5 porch+
		// 6 porch++
		GLASSERT(porch >= 0 && porch < NUM_PORCH);
		isPorch = porch;
		GLASSERT(isPorch == porch);
	}

	void SetLand( int _land )	{ 
		land = _land;
		GLASSERT(land >= WATER && land <= PORT);
	}

	void SetLandAndRock( U8 h )	{
		GLASSERT(sizeof(WorldGrid) == sizeof(U32)* 2);	// WorldGrid can be any size; just make sure it is the one intended 
		// So confusing. Max rock height=3, but land goes from 1-4 to be distinct from water.
		// Subtract here.
		if ( !h ) {
			SetWater();	
		}
		else {
			SetLand(LAND);
			SetNominalRockHeight( h-1 );
		}
	}

	int NominalRockHeight() const { return nominalRockHeight; }
	void SetNominalRockHeight( int h ) {
		GLASSERT( IsLand() );
		GLASSERT( h >= 0 && h <= MAX_ROCK_HEIGHT );
		nominalRockHeight = h;
	}

	// Is this something that can be hit? Interect with?
	bool IsObject() const { return rockHeight || plant;  }

	int RockHeight() const { return rockHeight; }
	void SetRockHeight( int h ) {
		GLASSERT( IsLand() || (h==0) );
		GLASSERT( h >= 0 && h <= MAX_ROCK_HEIGHT );
		GLASSERT(h == 0 || Plant() == 0);
		rockHeight = h;
		if ( h > 0 ) pave = 0;	// rock clears out pavement.
		if (h*FLUID_PER_ROCK >= (int)fluidHeight) fluidHeight = 0; // and clears out water it replaces
	}

	int RockType() const { return rockType; }
	void SetRockType( int type ) {
		GLASSERT( type >=0 && type <= ICE );
		rockType = type;
	}

	int Height() const {
		return grinliz::Max(fluidHeight/FLUID_PER_ROCK, int(rockHeight));
	}

	bool IsFluid() const {
		return fluidHeight > rockHeight * FLUID_PER_ROCK;
	}
	float FluidHeight() const {
		return float(fluidHeight) / float(FLUID_PER_ROCK);
	}
	int FluidHeightI() const {
		return fluidHeight;
	}

	bool IsFluidEmitter() const { return fluidEmitter ? true : false; }
	void SetFluidEmitter(bool on, int type) {
		GLASSERT(!FluidSink());
		fluidEmitter = on ? 1 : 0;
		fluidType = type;
	}

	int FluidType() const { return fluidType; }
	void SetFluidType(int type) {
		if (!fluidEmitter) {
			fluidType = type;
		}
	}

	bool IsWater() const		{ return !IsLand(); }
	void SetWater()				{ 
		land = WATER;
		nominalRockHeight = 0;
	}

	bool Magma() const			{ return magma != 0; }
	void SetMagma( bool m )		{ magma = m ? 1 : 0; if ( magma ) pave = 0; }

	// 1-based plant interpretation
	int Plant() const { return plant; }
	bool BlockingPlant() const { return plant && stage >= 2; }
	// 0-based
	int PlantStage() const { return stage;  }

	void SetPlant(int _type1based, int _stage)	{
		GLASSERT(_type1based == 0 || rockHeight == 0);
		GLASSERT(_type1based >= 0 && _type1based <= 8);	// 0 removes the plant
		plant = _type1based;
		stage = _stage;
	}

	void SetOn(bool onFire, bool onShock) {
		plantOnFire = onFire ? 1 : 0;
		plantOnShock = onShock ? 1 : 0;
	}
	void SetPlantOnFire(bool onFire) {
		plantOnFire = onFire ? 1 : 0;
	}
	void SetPlantOnShock(bool onShock) {
		plantOnShock = onShock ? 1 : 0;
	}
	bool PlantOnFire() const { return plantOnFire > 0; }
	bool PlantOnShock() const { return plantOnShock > 0; }

	int TotalHP() const			{ return plant ? ((stage + 1)*(stage + 1)) * HP_PER_PLANT_STAGE : rockHeight * HP_PER_HEIGHT; }
	int HP() const				{ return hp; }
	float HPFraction() const	{ return TotalHP() ? float(hp) / float(TotalHP()) : 0; }
	void DeltaHP( int delta ) {
		int points = hp;
		points += delta;
		if ( points < 0 ) points = 0;
		if ( points > TotalHP() ) points = TotalHP();
		hp = points;
		GLASSERT( hp == points );
	}
	void SetHP(int _hp) {
		hp = _hp;
		hp = grinliz::Clamp(hp, (unsigned)0, (unsigned)TotalHP());
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
		grinliz::Vector2I v = { int(x & mask), int(y & mask) };
		return v;
	}

	void SetZoneSize( int s )	{ 
		zoneSize = s;
		GLASSERT( s == zoneSize );
	}

	void SetPath( int pathData )	{ path = pathData; }

	// See SectorData for order.
	enum { PS_CORE, PS_NEG_X, PS_POS_X, PS_NEG_Y, PS_POS_Y, NUM_DEST };

	const grinliz::Vector2I& Path( int dest ) const {
		static const grinliz::Vector2I DIR[4] = {
			{1,0}, {0,1}, {-1,0}, {0,-1}
		};
		int index = (path >> (dest*2)) & 3;
		return DIR[index];
	}

};

#endif // LUMOS_WORLD_GRID_INCLUDED
