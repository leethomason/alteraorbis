#ifndef LUMOS_WORLDGEN_INCLUDED
#define LUMOS_WORLDGEN_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"

#include "../xegame/xegamelimits.h"

#include "../tinyxml2/tinyxml2.h"

namespace grinliz {
class PerlinNoise;
};

struct WorldFeature {
	int  id;						// unique id
	bool land;						// true: continent or island, false: ocean
	grinliz::Rectangle2I bounds;	// bounds
	int area;						// actual area

	void Save( tinyxml2::XMLPrinter* );
	void Load( const tinyxml2::XMLElement& element );

	class Compare
	{
	public:
		static bool Less( const WorldFeature& s0, const WorldFeature& s1 ) { return s0.area > s1.area; }
	};
};


class WorldGen
{
public:
	WorldGen();
	~WorldGen();

	// Done in order:
	//  - Start()
	//  - Do() for each Y
	//  - End()
	//  - optional: WriteMarker()
	void StartLandAndWater( U32 seed0, U32 seed1 );
	void DoLandAndWater( int y );
	bool EndLandAndWater( float fractionLand );
	void WriteMarker();

	void ApplyHeight( int x, int y, int h, int blend ) {
		int index = y*SIZE+x;
		GLASSERT( blend >= 0 && blend <= 256 );
		if ( land[index] ) {
			if ( blend >= 0 ) {
				land[index] = ((land[index] * (256-blend)) + (h * blend)) >> 8;
			}
			else {
				if ( land[index] ) 
					land[index] = h;
			}
			if ( land[index] == 0 )
				land[index] = 1;
		}
	}

	bool CalColor( grinliz::CDynArray<WorldFeature>* featureArr );

	// SIZExSIZE
	const U8* Land() const						{ return land; }
	const U8* Color() const						{ return color; }

	enum {
		SIZE = MAX_MAP_SIZE
	};

private:
	int CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh );
	void DrawCanal( grinliz::Vector2I v, int radius, int dx, int dy, const grinliz::Rectangle2I& wf );

	float* flixels;
	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	U8* land;	// 0: water, 1: last
	U8* color;	// 0: not processed, >0 color
	//grinliz::CDynArray< WorldFeature > featureArr;
};

#endif // LUMOS_WORLDGEN_INCLUDED