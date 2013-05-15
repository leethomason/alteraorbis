#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcolor.h"

static const int MAX_ROCK_HEIGHT		= 3;
static const int MHP_PER_HEIGHT			= 40;

struct WorldGrid {

private:
	// memset(0) should work, and make it water.

	unsigned isLand				: 1;
	unsigned isGrid				: 1;
	unsigned isPort				: 1;
	unsigned isCore				: 1;

	unsigned magma				: 1;	// land, rock, or water can be set to magma

	unsigned zoneSize			: 5;	// 0-31 (need 0-16)
	unsigned nominalRockHeight	: 2;	// 0-3
	unsigned rockHeight			: 2;
	unsigned poolHeight			: 2;

	unsigned mhp				: 7;	// 0-127, want something in the 0-100 range. Mega HP

	unsigned debugAdjacent		: 1;
	unsigned debugPath			: 1;
	unsigned debugOrigin		: 1;

	bool IsBlocked() const			{ return (!isLand) || isGrid || rockHeight || poolHeight; }

public:
	grinliz::Color4U8 ToColor() const {
		grinliz::Color4U8 c = { 0, 0, 0, 255 };

		if ( isGrid ) {
			c.Set( 120, 180, 180, 255 );
		}
		else if ( isPort ) {
			c.Set( 180, 180, 0, 255 );
		}
		else if ( isCore ) {
			c.Set( 200, 100, 100, 255 );
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
	bool IsCore() const			{ return isCore != 0; }
	bool IsGrid() const			{ return isGrid != 0; }

	enum {
		WATER,
		GRID,
		PORT,
		CORE,
		LAND,
		NUM_LAYERS
	};
	int Layer() const			{ 
		int layer = WATER;
		if ( isGrid ) layer = GRID;
		else if ( isPort ) layer = PORT;
		else if ( isCore ) layer = CORE;
		else if ( isLand ) layer = LAND;
		return layer;
	}

	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }

	void SetGrid()				{ isLand = 1; isGrid = 1; }
	void SetPort()				{ isLand = 1; isPort = 1; }
	void SetCore()				{ isLand = 1; isCore = 1; }
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
		GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); 
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
	}

	int PoolHeight() const { return poolHeight; }
	void SetPoolHeight( int p ) {
		GLASSERT( IsLand() || (p==0) );
		GLASSERT( !p || p > (int)rockHeight );
		poolHeight = p;
	}

	bool IsWater() const		{ return !IsLand(); }
	void SetWater()				{ 
		isLand = 0;
		nominalRockHeight = 0;
	}

	bool Magma() const			{ return magma != 0; }
	void SetMagma( bool m )		{ magma = m ? 1 : 0; }

	int TotalMHP() const		{ return rockHeight * MHP_PER_HEIGHT; }
	int MHP() const				{ return mhp; }
	void DeltaMHP( int delta ) {
		int points = mhp;
		points += delta;
		if ( points < 0 ) points = 0;
		if ( points > TotalMHP() ) points = TotalMHP();
		mhp = points;
		GLASSERT( mhp == points );
	}

	bool IsPassable() const { 
		return IsLand() && !IsBlocked(); 
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
};

#endif // LUMOS_WORLD_GRID_INCLUDED
