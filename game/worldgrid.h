#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcolor.h"

static const int MAX_ROCK_HEIGHT		= 3;

struct WorldGrid {
private:
	// memset(0) should work, and make it water.

	U8 pathColor;
	U8 pad0;
	U8 pad1;
	U8 pad2;

	unsigned isLand				: 1;
	unsigned isBlocked			: 1;

	unsigned zoneSize			: 6;	// 0-31
	unsigned nominalRockHeight	: 2;	// 0-3
	unsigned rockHeight			: 2;
	unsigned poolHeight			: 2;

	unsigned debugAdjacent		: 1;
	unsigned debugPath			: 1;
	unsigned debugOrigin		: 1;

public:
	grinliz::Color4U8 ToColor() const {
		grinliz::Color4U8 c = { 0, 0, 0, 255 };
#if 1
		if ( isLand && nominalRockHeight > 0 ) {
			U8 gr = 80*nominalRockHeight;
			c.Set( gr, gr, gr, 255 );
		}
		else
#endif
		{
			c.Set(	(17*pathColor) & 127,	
					(isLand * 0xc0),		//| (pathColor & 0x3f),
					(1-isLand) * 0xff,
					255 );
		}
		return c;
	}

	bool IsLand() const			{ return isLand != 0; }
	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }
	void SetLandAndRock( U8 h )	{
		if ( !h ) {
			SetWater();	
		}
		else {
			SetLand();
			SetNominalRockHeight( 0 );
			if ( h > 1 ) {
				// 2-85    -> 1
				// 86-170  -> 2
				// 171-255 -> 3
				static const int DELTA = 255 / MAX_ROCK_HEIGHT; // 63
				static const int BIAS = DELTA / 2;

				SetNominalRockHeight( 1 + (h-1)/DELTA );
			}
		}
	}

	void SetLand()				{ 
		GLASSERT( sizeof(WorldGrid) == 2*sizeof(U32) ); 
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
		GLASSERT( IsLand() );
		GLASSERT( h >= 0 && h <= MAX_ROCK_HEIGHT );
		rockHeight = h;
	}

	int PoolHeight() const { return poolHeight; }
	void SetPoolHeight( int p ) {
		GLASSERT( IsLand() );
		GLASSERT( !p || p > (int)rockHeight );
		poolHeight = p;
	}

	bool IsWater() const		{ return !IsLand(); }
	void SetWater()				{ 
		GLASSERT( sizeof(WorldGrid) == 2*sizeof(U32) );
		isLand = 0;
		nominalRockHeight = 0;
	}

	U32  PathColor() const		{ return pathColor; }
	void SetPathColor( int c )	{ 
		GLASSERT( c >= 0 && c <= 255 ); 
		pathColor = c;
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

	bool IsBlocked() const			{ return isBlocked != 0; }
	void SetBlocked( bool block )	{ 
		GLASSERT( IsLand() );
		if (block) {
			GLASSERT( IsPassable() );
			isBlocked = 1;
		}
		else { 
			isBlocked = 0;
		}
	}

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
