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


#define XENOENGINE_INSTANCING
#define EL_USE_VBO			// Use VBOs: a good thing, everywhere but the original iPhone
#define EL_USE_MRT_BLUR


enum HitTestMethod 
{
	TEST_HIT_AABB,
	TEST_TRI,
};


enum {
	EL_RES_NAME_LEN			= 24,
	EL_MAX_VERTEX_IN_GROUP	= 4096,
	EL_MAX_INDEX_IN_GROUP	= 4096,
	EL_MAX_MODEL_GROUPS		= 4,
	EL_MAX_VERTEX_IN_MODEL	= EL_MAX_VERTEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_MAX_INDEX_IN_MODEL	= EL_MAX_INDEX_IN_GROUP * EL_MAX_MODEL_GROUPS,
	EL_ALLOCATED_MODELS		= 4000,
	EL_FILE_STRING_LEN		= 24,
	EL_MAX_METADATA			= 4,		// both animation and model
	EL_MAX_MODEL_EFFECTS	= 4,		// max # of particle effects emitted by model
	EL_MAX_BONES			= 12,		// could be 16?
	EL_MAX_ANIM_FRAMES		= 15,
	EL_MAX_MAP_SIZE			= 1024,		// used for allocating hash tables and lookup structures

	// performance tuning
	EL_MAX_INSTANCE			= 16,		// Max instances used. Impacts # of uniforms.
	EL_TUNE_INSTANCE_MEM	= 8*1024	// Max memory per model.
};


static const float EL_FOV  = 40.0f;
static const float EL_NEAR = 2.0f;
static const float EL_FAR  = 240.0f;
static const float EL_CAMERA_MIN = 4.0f;
static const float EL_CAMERA_MAX = 140.0f;

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

#endif