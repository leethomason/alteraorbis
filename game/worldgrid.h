#ifndef LUMOS_WORLD_GRID_INCLUDED
#define LUMOS_WORLD_GRID_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"

// Trick: overlay the color (RGBA U8) and 
// the pathing data. That way there is a clean
// way to save, and do basic checks, on the
// information. 
// So: Color4U8* c = (Color4U8*)grid works.
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
	// Red Channel
	U32 isBlock			: 1;
public:
	U32 debug_adjacent  : 1;
	U32 debug_origin    : 1;
	U32 debug_path		: 1;
private:
	U32	pad0			: 1;
	U32 highRed			: 3;	// Magma: 0xe0
	// Green					
	U32 zoneColorLow	: 4;
	U32 green			: 4;	// Land: 0xf0
	// Blue						
	U32 zoneColorHigh	: 4;	
	U32 blue			: 4;	// Water: 0xf0
	// Alpha
	U32	size			: 5;	// zone size
	U32 alpha			: 3;	// 0xe0

public:
	bool IsBlock() const		{ return isBlock != 0; }
	bool IsLand() const			{ return green != 0; }
	bool IsPassable() const		{ return green && !isBlock; }
	U32  ZoneSize()  const		{ return size; }
	U32  ZoneColor() const		{ return zoneColorLow | (zoneColorHigh << 4); }

	void SetBlock( bool block )	{ isBlock = block ? 1 : 0; }
	void SetLand( bool land )	{ if ( land ) SetLand(); else SetWater(); }
	void SetLand()				{ GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); blue = 0; green = 0xf; alpha=0x7; }
	void SetWater()				{ GLASSERT( sizeof(WorldGrid) == sizeof(U32) ); blue = 0xf; green = 0; alpha=0x7; }
	void SetZoneColor( int c )	{ GLASSERT( c >= 0 && c <= 255 ); zoneColorLow = c & 0xf; zoneColorHigh = (c>>4)&0xf; }
	void SetZoneSize( U32 s )	{ size = s; }
};


#endif // LUMOS_WORLD_GRID_INCLUDED
