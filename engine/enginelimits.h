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

#ifndef ENGINELIMITS_INCLUDED
#define ENGINELIMITS_INCLUDED


#define EL_USE_VBO			// Use VBOs: a good thing, everywhere but the original iPhone
#define EL_USE_MRT_BLUR


enum HitTestMethod 
{
	TEST_HIT_AABB,
	TEST_TRI,
};


enum {
	EL_MAX_VERTEX_IN_GROUP	= 4096,
	EL_MAX_INDEX_IN_GROUP	= 4096,
	EL_MAX_MODEL_GROUPS		= 4,
	EL_MAX_VERTEX_IN_MODEL	= EL_MAX_VERTEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_MAX_INDEX_IN_MODEL	= EL_MAX_INDEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_FILE_STRING_LEN		= 30,
	EL_MAX_MODEL_EFFECTS	= 4,		// max # of particle effects emitted by model
	EL_MAX_BONES			= 16,		// There is shader uniform memory allocated to: MAX_BONES*MAX_INSTANCE, so be careful with those constants.
	EL_MAX_ANIM_FRAMES		= 24,

	EL_MAP_SIZE				= 1024,		// used for allocating hash tables and lookup structures
	EL_MAP_Y_SHIFT			= 10,
	EL_MAP_X_MASK			= 1023,	

	// performance tuning
	EL_MAX_INSTANCE			= 16,		// Max instances used. Impacts # of uniforms.
};


// It would be cool if this was a flexible, general system.
// Which was the first version. The first version was also
// a giant PITA, so I switched to a simple enum.
// The hardpoint and metadata system is a little cross-wired as well.
// You can't attached to a "target" hardpoint, which is convenient
// since a hardpoint==0 (META_TARGET) is no hardpoint
enum {
	META_TARGET,
	HARDPOINT_TRIGGER,	// this attaches to the trigger hardpoint
	HARDPOINT_ALTHAND,	// this attaches to the alternate hand (non-trigger) hardpoint. not always a hand.
	HARDPOINT_HEAD,		// this attaches to the head hardpoint
	HARDPOINT_SHIELD,	// this attaches to the shield hardpoint
	EL_NUM_METADATA
};

const char* IDToMetaData( int id );
int MetaDataToID( const char* );


static const float MAP_HEIGHT = 4.0f;


static const float EL_FOV  = 40.0f;
static const float EL_NEAR = 2.0f;
static const float EL_FAR  = 140.0f;
static const float EL_CAMERA_MIN = 4.0f;
static const float EL_CAMERA_MAX = 40.0f;

// Set the zoom. The zoom is in a range of 0.1-5.0, based on
// distance from a hypothetical bitmap. Zoom is passed in
// as a relative value.
static const float GAME_ZOOM_MIN = 0.1f;
static const float GAME_ZOOM_MAX = 5.0f;

// --- Debugging --- //
//#define SHOW_FOW			// visual debugging
//#define EL_SHOW_MODELS
//#define EL_SHOW_ALL_UNITS

// --- Performance -- //
// Pain to implement. Less interesting than the "using all the uniforms there are" approach.
//#define EL_VEC_BONES	// use vector pos & rot for bones, instead of mat4, to save uniform space in shader

#endif