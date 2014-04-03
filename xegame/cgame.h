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

#ifndef GAME_ADAPTOR_INCLUDED
#define GAME_ADAPTOR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// --- Platform to Core --- //
void* NewGame( int width, int height, int rotation );
void DeleteGame( void* handle );	// does not save! use GameSave if needed.

void GameDeviceLoss( void* handle );
void GameResize( void* handle, int width, int height, int rotation );
void GameSave( void* handle );

// Input
// Mimics the iPhone input. UFOAttack procesess:
//		Touch and drag. (scrolling, movement)
//		2 finger zoom in/out
//		Taps (buttons, UI)
//


#define GAME_TAP_DOWN		0
#define GAME_TAP_MOVE		1
#define GAME_TAP_UP			2
#define GAME_TAP_CANCEL		3
#define GAME_MOVE_WHILE_UP	4	// not on tablets, obviously
#define GAME_TAP_MOD_SHIFT	5	// debugging
#define GAME_TAP_MOD_CTRL	6	// debugging

void GameTap( void* handle, int action, int x, int y, int mod );

#define GAME_ZOOM_DISTANCE	0
#define GAME_ZOOM_PINCH		1
void GameZoom( void* handle, int style, float zoom );

#define GAME_PAN_START		7
#define GAME_PAN_MOVE		8
#define GAME_PAN_END		9
void GameCameraPan(void* handle, int action, float x, float y);

// Relative rotation, in degrees.
void GameCameraRotate(void* handle, float degrees);
void GameDoTick( void* handle, unsigned int timeInMSec );

#define GAME_HK_TOGGLE_UI				1
#define GAME_HK_TOGGLE_DEBUG_TEXT		2
#define GAME_HK_TOGGLE_DEBUG_UI			3
#define GAME_HK_TOGGLE_PERF				4
#define GAME_HK_SPACE					5	// general action
#define GAME_HK_TOGGLE_FAST				6	// fast mode and normal mode
#define GAME_HK_TOGGLE_PATHING			7
#define GAME_HK_TOGGLE_COLORS			8
#define GAME_HK_MAP					    9
#define GAME_HK_CHEAT_GOLD			   10
#define GAME_HK_CHEAT_CRYSTAL		   11
#define GAME_HK_CHEAT_ELIXIR		   12
#define GAME_HK_CHEAT_TECH			   13
#define GAME_HK_CHEAT_HERD			   14

#define GAME_HK_TOGGLE_GLOW			   20
#define GAME_HK_TOGGLE_PARTICLE		   21
#define GAME_HK_TOGGLE_VOXEL		   22
#define GAME_HK_TOGGLE_SHADOW		   23
#define GAME_HK_TOGGLE_BOLT			   24

void GameHotKey( void* handle, int value );

#define GAME_MAX_MOD_DATABASES			16
void GameAddDatabase( const char* path );

int GamePopSound( void* handle, int* databaseID, int* offset, int* size );	// returns 1 if a sound was available

// --- Core to platform --- //
void PlatformPathToResource( char* buffer, int bufferLen, int* offset, int* length );
const char* PlatformName();
//void PlayWAVSound( int offset, int nBytes );

enum {
	GAME_APP_DIR,
	GAME_SAVE_DIR,
};

namespace grinliz {
	class GLString;
};

// Get a system path. The directory will be created, if
// needed, but the path is a standard fopen(). And
// hopefully fopen() is portable enough. I'm sure I'll
// revisit this for the 50th time.
void GetSystemPath(int root, const char* filename, grinliz::GLString* out);


// ----------------------------------------------------------------
// Debugging and adjustment
//enum {
//	GAME_CAMERA_TILT,
//	GAME_CAMERA_YROTATE,
//	GAME_CAMERA_ZOOM
//};
//void GameCameraGet( void* handle, int param, float* value );
//void GameMoveCamera( void* handle, float dx, float dy, float dz );
	
#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
