#ifndef LUMOS_PROCEDURAL_INCLUDED
#define LUMOS_PROCEDURAL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../xegame/game.h"


// http://chir.ag/projects/name-that-color/
enum {
	PAL_ZERO,
	PAL_RED,
	PAL_TANGERINE,
	PAL_GREEN,
	PAL_BLUE,
	PAL_PURPLE,
	PAL_GRAY,
	PAL_COUNT
};

class FaceGen
{
public:

	FaceGen( const Game::Palette* p_palette ) : palette( p_palette ) {}

	enum { 
		NUM_SKIN_COLORS = 3,
		NUM_HAIR_COLORS	= 10,
		NUM_GLASSES_COLORS = 4
	};
	void GetSkinColor( int index0, int index1, float fade, grinliz::Color4F* c, grinliz::Color4F* highlight );
	void GetHairColor( int index0, grinliz::Color4F* c );
	void GetGlassesColor( int index0, int index1, float fade, grinliz::Color4F* c );
	
	enum {
		SKIN, HIGHLIGHT, GLASSES, HAIR
	};
	void GetColors( U32 seed, grinliz::Color4F* c );

private:
	const Game::Palette* palette;
};


#endif // LUMOS_PROCEDURAL_INCLUDED
