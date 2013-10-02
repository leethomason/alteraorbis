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
void* NewGame( int width, int height, int rotation, const char* savePath );
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
#define GAME_TAP_MOVE_UP	4	// debugging; not on tablets, obviously
#define GAME_TAP_MOD_SHIFT	5	// debugging
#define GAME_TAP_MOD_CTRL	6	// debugging

#define GAME_TAP_MASK		0x00ff
#define GAME_TAP_PANNING	0x0100
#define GAME_TAP_DOWN_PANNING		(GAME_TAP_DOWN | GAME_TAP_PANNING)
#define GAME_TAP_MOVE_PANNING		(GAME_TAP_MOVE | GAME_TAP_PANNING)
#define GAME_TAP_UP_PANNING			(GAME_TAP_UP | GAME_TAP_PANNING)
#define GAME_TAP_CANCEL_PANNING		(GAME_TAP_CANCEL | GAME_TAP_PANNING)

void GameTap( void* handle, int action, int x, int y, int mod );

#define GAME_ZOOM_DISTANCE	0
#define GAME_ZOOM_PINCH		1
void GameZoom( void* handle, int style, float zoom );

// Relative rotation, in degrees.
void GameCameraRotate( void* handle, float degrees );
void GameDoTick( void* handle, unsigned int timeInMSec );

#define GAME_HK_TOGGLE_UI				1
#define GAME_HK_TOGGLE_DEBUG_TEXT		2
#define GAME_HK_TOGGLE_PERF				3
#define GAME_HK_TOGGLE_GLOW				4
#define GAME_HK_SPACE					5	// general action
#define GAME_HK_TOGGLE_FAST				6	// fast mode and normal mode
#define GAME_HK_DEBUG_UI				7
#define GAME_HK_TOGGLE_PATHING			8
#define GAME_HK_TOGGLE_COLORS			9
#define GAME_HK_MAP					   10

void GameHotKey( void* handle, int value );

#define GAME_MAX_MOD_DATABASES			16
void GameAddDatabase( const char* path );

int GamePopSound( void* handle, int* databaseID, int* offset, int* size );	// returns 1 if a sound was available

// --- Core to platform --- //
void PlatformPathToResource( char* buffer, int bufferLen, int* offset, int* length );
const char* PlatformName();
//void PlayWAVSound( int offset, int nBytes );

// ----------------------------------------------------------------
// Debugging and adjustment
enum {
	GAME_CAMERA_TILT,
	GAME_CAMERA_YROTATE,
	GAME_CAMERA_ZOOM
};
//void GameCameraGet( void* handle, int param, float* value );
//void GameMoveCamera( void* handle, float dx, float dy, float dz );
	
#ifdef __cplusplus
}
#endif

#endif	// GAME_ADAPTOR_INCLUDED
