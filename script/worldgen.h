#ifndef LUMOS_WORLDGEN_INCLUDED
#define LUMOS_WORLDGEN_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrectangle.h"
#include "../xegame/xegamelimits.h"


struct WorldFeature {
	int  id;						// unique id
	bool land;						// true: continent, island, false: ocean
	grinliz::Rectangle2I bounds;	// bounds & area
};


class WorldGen
{
public:
	WorldGen();
	~WorldGen();

	bool CalcLandAndWater( U32 seed0, U32 seed1, float fractionLand );
	bool CalColor();

	// SIZExSIZE
	const U8* Land() const	{ return land; }
	const U8* Color() const	{ return color; }

	enum {
		SIZE = MAX_MAP_SIZE
	};

private:
	int CountFlixelsAboveCutoff( const float* flixels, float cutoff );

	U8* land;	// 0: water, 1: last
	U8* color;	// 0: not processed, >0 color
};

#endif // LUMOS_WORLDGEN_INCLUDED