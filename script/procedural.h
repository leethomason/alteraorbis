/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LUMOS_PROCEDURAL_INCLUDED
#define LUMOS_PROCEDURAL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
#include "../engine/texture.h"
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

class GameItem;

struct ProcRenderInfo
{
	ProcRenderInfo() {
		texture = 0;
		for( int i=0; i<4; ++i ) {
			filter[i] = true;
		}
	}

	Texture*			texture;
	Texture::TableEntry	te;
	grinliz::Matrix4	color;
	grinliz::IString	filterName[4];
	bool				filter[4];		// item filters - maps to PROC_RING_MAIN, etc.
};


class HumanGen
{
public:
	HumanGen( bool female, U32 seed, int team, bool electric );

	// False colors used by the face:
	// red: skin
	// green: hair
	// blue: glasses/tattoo
	void AssignFace( ProcRenderInfo* info );
	// red: skin
	// green: hair
	// blue: suit
	void AssignSuit( ProcRenderInfo* info );

private:

	void GetSkinColor( int index, grinliz::Color4F* c );
	void GetHairColor( int index, grinliz::Color4F* c );
	void GetGlassesColor( int index, grinliz::Color4F* c );
	void GetSuitColor( grinliz::Vector4F* c );
	
	enum {
		SKIN, HAIR, GLASSES
	};
	void GetColors( grinliz::Vector4F* v );

	bool female;
	int  team;
	U32  seed;
	bool electric;
};


class WeaponGen
{
public:

	enum {
		GUN_CELL	= 0x02,	// boosts clip capacity
		GUN_DRIVER	= 0x04,	// boosts power/damage
		GUN_SCOPE	= 0x08,	// boosts accuracy

		RING_GUARD	= 0x02,	// shield coupler
		RING_TRIAD	= 0x04,	// boosts damage
		RING_BLADE	= 0x08,	// boosts damage, decrease shield coupling

		DENIZEN_RING_PART_MASK = RING_GUARD | RING_TRIAD,
		TROLL_RING_PART_MASK   = RING_BLADE,

		PART_MASK   = 0x0f
	};

	WeaponGen(U32 _seed, int _team, int _effectFlags, int _features);
	void AssignRing( ProcRenderInfo* info );
	void AssignGun( ProcRenderInfo* info );
	void AssignShield( ProcRenderInfo* info );

	// Only weapon-gen specific in that the alpha is set.
	static grinliz::Color4F GetEffectColor(int effect);

private:
	void GetColors( int effectFlags, grinliz::Color4F* array );
	enum {
		BASE,
		CONTRAST,
		EFFECT,

		NUM_COLORS = 11,
		NUM_ROWS = 8
	};
	U32 seed;
	int team;	// the group part
	int effectFlags;
	int features;
};


class TeamGen
{
public:
	static void Assign( U32 seed, int team, ProcRenderInfo* info );
	static void TeamBuildColors(int team, grinliz::Vector2I* base, grinliz::Vector2I* contrast, grinliz::Vector2I* glow);
	static void TeamWeaponColors(int team, grinliz::Vector2I* base, grinliz::Vector2I* contrast);
};


// Easier form.
void AssignProcedural(const GameItem* item, ProcRenderInfo* info);

void AssignProcedural( const grinliz::IString& procedure,
					   bool female, 
					   U32 seed,			// random #
					   int team,			// team, full value (id & group)
					   bool electric,		// more colorface face rendering
					   int effectFlags,		// EFFECT_FIRE etc.
					   int features,		// which parts of the weapon are in use
					   ProcRenderInfo* info );

#endif // LUMOS_PROCEDURAL_INCLUDED
