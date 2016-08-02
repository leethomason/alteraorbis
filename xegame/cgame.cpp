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


#include "../libs/SDL2/include/SDL.h"
#include "../grinliz/gldebug.h"
#include "cgame.h"
#include "../game/lumosgame.h"


#if defined( UFO_WIN32_SDL )
#include "../libs/SDL2/include/SDL_filesystem.h"
#include <Shlobj.h>
static const char* winResourcePath = "./res/lumos.db";
#endif

#ifdef UFO_LINUX_SDL
#include <sys/stat.h>
#endif

#ifdef UFO_IPHONE
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef ANDROID_NDK
extern "C" int androidResourceOffset;
extern "C" int androidResourceLen;
extern "C" char androidResourcePath[200];
#endif

#include "../grinliz/glstringutil.h"


class CheckThread
{
public:
	CheckThread()	{ GLASSERT( active == false ); active = true; }
	~CheckThread()	{ GLASSERT( active == true ); active = false; }
private:
	static bool active;
};

bool CheckThread::active = false;

static char* modDatabases[ GAME_MAX_MOD_DATABASES ] = { 0 };


void* NewGame( int width, int height, int rotation )
{
	CheckThread check;

	LumosGame* game = new LumosGame( width, height, rotation );
	GLOUTPUT(( "NewGame.\n" ));

	return game;
}


void DeleteGame( void* handle )
{
	CheckThread check;

	GLOUTPUT(( "DeleteGame. handle=%p\n", handle ));
	if ( handle ) {
		Game* game = (Game*)handle;
		delete game;
	}
	for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
		if ( modDatabases[i] ) {
			Free( modDatabases[i] );
			modDatabases[i] = 0;
		}
	}
}



void GameResize( void* handle, int width, int height, int rotation ) {
	CheckThread check;

	GLOUTPUT(( "GameResize. handle=%p\n", handle ));
	Game* game = (Game*)handle;
	game->Resize( width, height, rotation );
}


void GameSave( void* handle ) {
	CheckThread check;

	GLOUTPUT(( "GameSave. handle=%p\n", handle ));
	Game* game = (Game*)handle;
	game->Save();
}


void GameDeviceLoss( void* handle )
{
	CheckThread check;

	GLOUTPUT(( "GameDeviceLoss. handle=%p\n", handle ));
	Game* game = (Game*)handle;
	game->DeviceLoss();
}


void GameZoom( void* handle, int style, float delta )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Zoom( style, delta );
}


void GameCameraRotate( void* handle, float degrees )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Rotate( degrees );
}


void GameCameraPan(void* handle, int action, float x, float y)
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Pan(action, x, y);
}


void GameCameraMove(void* handle, float dx, float dy)
{
	CheckThread check;
	Game* game = (Game*)handle;
	game->MoveCamera(dx, dy);
}


void GameTap(void* handle, int action, int x, int y, int mod)
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->Tap( action, x, y, mod );
}


void GameDoTick( void* handle, unsigned int timeInMSec )
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->DoTick( timeInMSec );
}

void GameFPSMove(void* handle, float forward, float right, float rotate)
{
	CheckThread check;

	Game* game = (Game*)handle;
	game->FPSMove(forward, right, rotate);
}


void GameHotKey( void* handle, int mask )
{
	CheckThread check;

	Game* game = (Game*)handle;
	return game->HandleHotKey( mask );
}



void PathToDatabase(char* buffer, int bufferLen, int* offset, int* length)
{
	*offset = 0;
	*length = 0;
#if defined( UFO_IPHONE )
	CFStringRef nameRef = CFStringCreateWithCString( 0, name, kCFStringEncodingWindowsLatin1 );
	CFStringRef extensionRef = CFStringCreateWithCString( 0, extension, kCFStringEncodingWindowsLatin1 );
	
	CFBundleRef mainBundle = CFBundleGetMainBundle();	
	CFURLRef imageURL = CFBundleCopyResourceURL( mainBundle, nameRef, extensionRef, NULL );
	if ( !imageURL ) {
		GLOUTPUT(( "Error loading '%s' '%s'\n", name, extension ));
	}
	GLASSERT( imageURL );
		
	CFURLGetFileSystemRepresentation( imageURL, true, (unsigned char*)buffer, bufferLen );
#elif defined( UFO_WIN32_SDL )
	grinliz::StrNCpy( buffer, winResourcePath, bufferLen );
#elif defined (ANDROID_NDK)
	grinliz::StrNCpy( buffer, androidResourcePath, bufferLen );
	*offset = androidResourceOffset;
	*length = androidResourceLen;
#elif defined (UFO_LINUX_SDL)
	grinliz::StrNCpy(buffer, "res/lumos.db", bufferLen);
#else
#	error UNDEFINED
#endif
}


const char* PlatformName()
{
#if defined( UFO_WIN32_SDL )
	return "pc";
#elif defined (ANDROID_NDK)
	return "android";
#elif defined (UFO_LINUX_SDL)
	return "linux";
#else
#	error UNDEFINED
#endif
}


void GameAddDatabase( const char* path )
{
	for( int i=0; i<GAME_MAX_MOD_DATABASES; ++i ) {
		if ( modDatabases[i] == 0 ) {
			int len = strlen( path ) + 1;
			modDatabases[i] = (char*)Malloc( len );
			strcpy( modDatabases[i], path );
			break;
		}
	}
}

