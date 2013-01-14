#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"

// The read-ony parts in the WorldGrid.
// The read-write parts are in the World

// Trick: overlay the color (RGBA U8) and 
// the pathing data. That way there is a clean
// way to save, and do basic checks, on the
// information. 
// So: Color4U8* c = (Color4U8*)grid works.
// Something I learned on this: bit field layout is complier dependent.
// That's a bummer.
//

struct WorldGrid {
private:
	// memset(0) should work, and make it water.
	U8 r, g, b, a;

	enum { 
		// Red
		RED				= 0xff,

		// Green
		GREEN			= 0xff,

		// Blue
		PATH_COLOR		= 0x7f,
		BLUE			= 0x80,

		// Alpha
		ALPHA			= 0xff
	};

public:
	bool IsLand() const			{ return (g & GREEN) != 0; }
	U32  PathColor() const		{ return b & PATH_COLOR; }

	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }
	void SetLand()				{ 
		GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); 
		r &= (~RED);
		b &= (~BLUE); 
		g |= GREEN; 
		a |= ALPHA; 
	}
	void SetWater()				{ 
		GLASSERT( sizeof(WorldGrid) == sizeof(U32) );
		r &= (~RED);
		b |= BLUE; 
		g &= (~GREEN); 
		a |= ALPHA; 
	}
	void SetPathColor( int c )	{ 
		GLASSERT( c >= 0 && c <= 127 ); 
		b &= (~PATH_COLOR);
		b |= c;
	}
};


struct WorldGridState
{
	enum {
		ZONE_SIZE_MASK	= 0x01f,	// use low bits to avoid twiddling 
		DEBUG_ADJACENT	= 0x020,
		DEBUG_ORIGIN	= 0x040,
		DEBUG_PATH		= 0x080,
		IS_BLOCKED		= 0x100,
	};


	static bool IsPassable( const WorldGrid& g, const WorldGridState& gs ) { 
		return g.IsLand() && !gs.IsBlocked(); 
	}

	bool DebugAdjacent() const	{ return (flags & DEBUG_ADJACENT) != 0; }
	void SetDebugAdjacent( bool v ) {
		flags &= (~DEBUG_ADJACENT);
		if ( v ) flags |= DEBUG_ADJACENT;
	}

	bool DebugOrigin() const	{ return (flags & DEBUG_ORIGIN) != 0; }
	void SetDebugOrigin( bool v ) {
		flags &= (~DEBUG_ORIGIN);
		if ( v ) flags |= DEBUG_ORIGIN;
	}

	bool DebugPath() const		{ return (flags & DEBUG_PATH) != 0; }
	void SetDebugPath( bool v ) {
		flags &= (~DEBUG_PATH);
		if ( v ) flags |= DEBUG_PATH;
	}

	bool IsBlocked() const		{ return (flags & IS_BLOCKED) != 0; }
	static void SetBlocked( const WorldGrid& g, WorldGridState* gs, bool block )	{ 
		GLASSERT( g.IsLand() );
		if (block) {
			GLASSERT( IsPassable( g, *gs) );
			gs->flags |= IS_BLOCKED; 
		}
		else { 
			gs->flags &= (~IS_BLOCKED); 
		}
	}

	U32 ZoneSize() const {
		return flags & ZONE_SIZE_MASK;
	}

	// x & y must be in zone.
	grinliz::Vector2I ZoneOrigin( int x, int y ) const {
		U32 mask = 0xffffffff;
		U32 size = ZoneSize();
		if ( size ) {
			mask = ~(size-1);
		}
		grinliz::Vector2I v = { x & mask, y & mask };
		return v;
	}

	void SetZoneSize( int s )	{ 
		flags &= (~ZONE_SIZE_MASK);
		flags |= s;
		GLASSERT( s == ( flags & ZONE_SIZE_MASK ));
	}

private:
	U16 flags;
};

#endif // LUMOS_WORLD_GRID_INCLUDED
