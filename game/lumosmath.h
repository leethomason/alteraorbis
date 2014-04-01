#ifndef LUMOS_MATH_INCLUDED
#define LUMOS_MATH_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrectangle.h"
#include "gamelimits.h"

inline grinliz::Vector3F ToWorld3F( const grinliz::Vector2I& pos2i ) {
	grinliz::Vector3F v = { float(pos2i.x) + 0.5f, 0.0f, float(pos2i.y) + 0.5f };
	return v;
}

inline grinliz::Vector2F ToWorld2F( const grinliz::Vector2I& pos2i ) {
	grinliz::Vector2F v = { float(pos2i.x) + 0.5f, float(pos2i.y) + 0.5f };
	return v;
}

inline grinliz::Vector2F ToWorld2F( const grinliz::Vector3F& pos3 ) {
	grinliz::Vector2F v = { pos3.x, pos3.z };
	return v;
}

inline grinliz::Vector2I ToWorld2I( const grinliz::Vector2F& pos2 ) {
	grinliz::Vector2I pos2i = { (int)pos2.x, (int)pos2.y };
	return pos2i;
}

inline grinliz::Vector2I ToWorld2I( const grinliz::Vector3F& pos3 ) {
	grinliz::Vector2I pos2i = { (int)pos3.x, (int)pos3.z };
	return pos2i;
}

inline grinliz::Rectangle2F ToWorld(const grinliz::Rectangle2I& ri)
{
	grinliz::Rectangle2F r;
	r.Set(float(ri.min.x), float(ri.min.y), float(ri.max.x + 1), float(ri.max.y + 1));
	return r;
}

inline grinliz::Vector2I AdjacentWorldGrid( const grinliz::Vector2F& pos2 ) {
	grinliz::Vector2I v = ToWorld2I( pos2 );

	float dx = pos2.x - (float(v.x) + 0.5f);
	float dy = pos2.y - (float(v.y) + 0.5f);
	if ( fabsf( dx > dy ))
		v.x += (dx>0) ? 1 : -1;
	else
		v.y += (dy>0) ? 1 : -1;
	return v;
}

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

inline grinliz::Rectangle2I SectorBounds( const grinliz::Vector2I& sector ) {
	grinliz::Rectangle2I r;
	r.Set( sector.x*SECTOR_SIZE, sector.y*SECTOR_SIZE,
		   (sector.x+1)*SECTOR_SIZE-1, (sector.y+1)*SECTOR_SIZE-1 );
	return r;
}

inline grinliz::Rectangle2I InnerSectorBounds( const grinliz::Vector2I& sector ) {
	grinliz::Rectangle2I r;
	r.Set( sector.x*SECTOR_SIZE+1, sector.y*SECTOR_SIZE+1,
		   (sector.x+1)*SECTOR_SIZE-2, (sector.y+1)*SECTOR_SIZE-2 );
	return r;
}

inline int SectorIndex( const grinliz::Vector2I& sector ) {
	GLASSERT( sector.x >= 0 && sector.y >= 0 && sector.x < NUM_SECTORS && sector.y < NUM_SECTORS );
	return sector.y * NUM_SECTORS + sector.x;
}


// Convert 3-18 (actually 1-20) to 0.4->2.0
// 10.5 is normal -> 1.0, how to map? The highest shouldn't be 20x the lowest, probably.
// Aim for a little moderation in the constants.
inline float Dice3D6ToMult( int dice ) {
	int d = grinliz::Max( 0, dice );

	float uniform = float(d) / 20.0f;	// [0,1+]
	float r = 1;
	if ( uniform < 0.5f ) {
		r = grinliz::Lerp( 0.3f, 1.0f, uniform*2.0f );
	}
	else {
		r = grinliz::Lerp( 1.0f, 2.0f, (uniform-0.5f)*2.0f );
	}
	return r;
}


#endif // LUMOS_MATH_INCLUDED

