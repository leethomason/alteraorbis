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
		texture = 0;
		for( int i=0; i<4; ++i ) {
			filterName[i] = grinliz::IString();
			filter[i] = true;
		}
		uv.Zero();
		clip.Zero();
	}

	Texture*			texture;
	grinliz::Matrix4	color;			// color of the layers
	grinliz::Vector4F	uv;				// texture uv
	grinliz::Vector4F	clip;			// texture clip
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
		XFORM_CLIP_MAP
	};
	static int RenderItem( int seed, const GameItem& item, ProcRenderInfo* info );
	// PROC_RING_GUARD -> "guard"
	static grinliz::IString ProcIDToName( int id );

	virtual void Render( int seed, const GameItem& item, ProcRenderInfo* info ) = 0;

protected:
	ItemGen() {}
};


// NOT a subclass. Face rendering is different from the other modes.
class FaceGen
{
public:

	FaceGen( bool p_female ) : female(p_female) {}

	enum { 
		NUM_SKIN_COLORS = 4,
		NUM_HAIR_COLORS	= 9,
		NUM_GLASSES_COLORS = 6,
	};
	void GetSkinColor( int index0, int index1, float fade, grinliz::Color4F* c );
	void GetHairColor( int index0, grinliz::Color4F* c );
	void GetGlassesColor( int index0, int index1, float fade, grinliz::Color4F* c );
	
	enum {
		SKIN, GLASSES, HAIR
	};
	void GetColors( U32 seed, grinliz::Color4F* c );
	void GetColors( U32 seed, grinliz::Vector4F* v );
	void Render( int seed, ProcRenderInfo* info );

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

		NUM_COLORS = 11,
		NUM_ROWS = 8
	};

	WeaponGen() : ItemGen() {}
	virtual void Render( int seed, const GameItem& item, ProcRenderInfo* info );

	// [base, contrast, effect, glow]
	void GetColors( int i, bool fire, bool shock, grinliz::Color4F* array );
};


#endif // LUMOS_PROCEDURAL_INCLUDED
