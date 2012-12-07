#ifndef LUMOS_PROCEDURAL_INCLUDED
#define LUMOS_PROCEDURAL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
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

enum {
	PROC_RING_MAIN	= 0,	// each group starts at 0
	PROC_RING_GUARD,
	PROC_RING_TRIAD,
	PROC_RING_BLADE,

	PROC_COUNT
};


class GameItem;

struct ProcRenderInfo
{
	ProcRenderInfo() {
		for( int i=0; i<4; ++i ) {
			color[i].Set( 1,1,1,1 );
			vOffset[i] = 0;
			filterName[i] = grinliz::IString();
			filter[i] = true;
		}
	}

	grinliz::Color4F	color[4];		// color of the layers
	float				vOffset[4];		// texture offset
	grinliz::IString	filterName[4];
	bool				filter[4];		// item filters - maps to PROC_RING_MAIN, etc.
};


class ItemGen
{
public:
	virtual ~ItemGen() {};

	static int ToID( grinliz::IString name );
	static grinliz::IString ToName( int id );

	// Could be a factory and all that, but just uses a switch.
	enum {
		NONE,
		COLOR_XFORM,
		PROC4
	};
	static int RenderItem( const Game::Palette* palette, const GameItem& item, ProcRenderInfo* info );
	// PROC_RING_GUARD -> "guard"
	static grinliz::IString ProcIDToName( int id );

	virtual void Render( const GameItem& item, ProcRenderInfo* info ) = 0;

protected:
	ItemGen( const Game::Palette* p_palette ) : palette( p_palette ) {}
	const Game::Palette* palette;
};


class FaceGen : public ItemGen
{
public:

	FaceGen( bool p_female, const Game::Palette* p_palette ) : ItemGen( p_palette ), female(p_female) {}

	enum { 
		NUM_SKIN_COLORS = 4,
		NUM_HAIR_COLORS	= 10,
		NUM_GLASSES_COLORS = 6,

		FACE_ROWS			= 16,
		EYE_ROWS			= 16,
		MALE_GLASSES_ROWS	= 11,
		FEMALE_GLASSES_ROWS	= 15,
		HAIR_ROWS			= 16
	};
	void GetSkinColor( int index0, int index1, float fade, grinliz::Color4F* c, grinliz::Color4F* highlight );
	void GetHairColor( int index0, grinliz::Color4F* c );
	void GetGlassesColor( int index0, int index1, float fade, grinliz::Color4F* c );
	
	enum {
		SKIN, HIGHLIGHT, GLASSES, HAIR
	};
	void GetColors( U32 seed, grinliz::Color4F* c );
	virtual void Render( const GameItem& item, ProcRenderInfo* info );

private:
	bool female;
};


class WeaponGen : public ItemGen
{
public:
	enum {
		BASE,
		CONTRAST,
		EFFECT,
		GLOW,

		NUM_COLORS = 11,
		NUM_ROWS = 4
	};

	WeaponGen( const Game::Palette* p_palette ) : ItemGen( p_palette ) {}
	virtual void Render( const GameItem& item, ProcRenderInfo* info );

	// [base, contrast, effect, glow]
	void GetColors( int i, bool fire, bool shock, grinliz::Color4F* array );
};


#endif // LUMOS_PROCEDURAL_INCLUDED
