#ifndef LUMOS_MATH_INCLUDED
#define LUMOS_MATH_INCLUDED

#include "../grinliz/glvector.h"
#include "gamelimits.h"

inline grinliz::Vector2I ToSector( const grinliz::Vector2I& pos2i ) {
	grinliz::Vector2I v = { pos2i.x / SECTOR_SIZE, pos2i.y / SECTOR_SIZE };
	GLASSERT( v.x >= 0 && v.x < NUM_SECTORS );
	GLASSERT( v.y >= 0 && v.y < NUM_SECTORS );
	return v;
}

inline grinliz::Vector2I ToSector( int x, int y ) {
	grinliz::Vector2I pos2i = { x, y };
	return ToSector( pos2i );
}

inline int SectorIndex( const grinliz::Vector2I& sector ) {
	GLASSERT( sector.x >= 0 && sector.y >= 0 && sector.x < NUM_SECTORS && sector.y < NUM_SECTORS );
	return sector.y * NUM_SECTORS + sector.x;
}


#endif // LUMOS_MATH_INCLUDED

