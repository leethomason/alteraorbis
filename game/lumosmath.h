#ifndef LUMOS_MATH_INCLUDED
#define LUMOS_MATH_INCLUDED

#include "../grinliz/glvector.h"
#include "gamelimits.h"

grinliz::Vector2I ToSector( const grinliz::Vector2I& pos2i ) {
	grinliz::Vector2I v = { pos2i.x / SECTOR_SIZE, pos2i.y / SECTOR_SIZE };
	return v;
}



#endif // LUMOS_MATH_INCLUDED

