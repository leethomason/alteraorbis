#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

// Trick: overlay the color (RGBA U8) and 
// the pathing data. That way there is a clean
// way to save, and do basic checks, on the
// information. 
// So: Color4U8* c = (Color4U8*)grid works.
// Something I learned on this: bit field layout is complier dependent.
// That's a bummer.
//
//						Red				Green		Blue		Alpha
//	Water				L:std flags		cz			H Set		0xff
//	Land				L:std flags		H Set		cz
//		rock			H: rock height & age
//		rock&lake		H: water height
//	Magma				L:std flags
//						0xf0

struct WorldGrid {
private:
	// memset(0) should work, and make it water.
	U8 r, g, b, a;

	enum { 
		// Red
		DEBUG_ADJACENT	= 0x01,
		DEBUG_ORIGIN	= 0x02,
		DEBUG_PATH		= 0x04,
		IS_BLOCKED		= 0x08,
		RED				= 0xf0,

		// Green
		ZONE_COLOR_LOW	= 0x0f,
		GREEN			= 0xf0,

		// Blue
		ZONE_COLOR_HIGH	= 0x0f,
		BLUE			= 0xf0,

		// Alpha
		ZONE_SIZE		= 0x1f,
		ALPHA			= 0xe0
	};

public:
	bool DebugAdjacent() const	{ return (r & DEBUG_ADJACENT) != 0; }
	bool DebugOrigin() const	{ return (r & DEBUG_ORIGIN) != 0; }
	bool DebugPath() const		{ return (r & DEBUG_PATH) != 0; }
	bool IsBlocked() const		{ return (r & IS_BLOCKED) != 0; }
	bool IsLand() const			{ return (g & GREEN) != 0; }
	bool IsPassable() const		{ return IsLand() && !IsBlocked(); }
	U32  ZoneSize()  const		{ return (a & ZONE_SIZE); }
	U32  ZoneColor() const		{ return (g & ZONE_COLOR_LOW) | ((b & ZONE_COLOR_HIGH)<< 4); }

	void SetDebugAdjacent( bool v ) {
		r &= (~DEBUG_ADJACENT);
		if ( v ) r |= DEBUG_ADJACENT;
	}
	void SetDebugOrigin( bool v ) {
		r &= (~DEBUG_ORIGIN);
		if ( v ) r |= DEBUG_ORIGIN;
	}
	void SetDebugPath( bool v ) {
		r &= (~DEBUG_PATH);
		if ( v ) r |= DEBUG_PATH;
	}

	void SetBlocked( bool block )	{ 
		if (block) {
			GLASSERT( IsPassable() );
			r |= IS_BLOCKED; 
		}
		else { 
			r &= (~IS_BLOCKED); 
		}
	}
	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }
	void SetLand()				{ 
		GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); 
		b &= (~BLUE); 
		g |= GREEN; 
		a |= ALPHA; 
	}
	void SetWater()				{ 
		GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); 
		b |= BLUE; 
		g &= (~GREEN); 
		a |= ALPHA; 
	}
	void SetZoneColor( int c )	{ 
		GLASSERT( c >= 0 && c <= 255 ); 
		g &= (~ZONE_COLOR_LOW);
		b &= (~ZONE_COLOR_HIGH);

		g |= (c & ZONE_COLOR_LOW);
		b |= ( (b>>4) & ZONE_COLOR_HIGH);
	}
	void SetZoneSize( U32 s )	{ 
		a &= (~ZONE_SIZE);
		a |= s;
	}
};


#endif // LUMOS_WORLD_GRID_INCLUDED
