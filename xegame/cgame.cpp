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

#include "../grinliz/gldebug.h"
#include "cgame.h"
#include "../game/lumosgame.h"


#if defined( UFO_WIN32_SDL )
#include "../libs/SDL2/include/SDL_filesystem.h"
#include <Shlobj.h>
static const char* winResourcePath = "./res/lumos.db";
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

	GLOUTPUT(( "DeleteGame. handle=%x\n", handle ));
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

	GLOUTPUT(( "GameResize. handle=%x\n", handle ));
	Game* game = (Game*)handle;
	game->Resize( width, height, rotation );
}


void GameSave( void* handle ) {
	CheckThread check;

	GLOUTPUT(( "GameSave. handle=%x\n", handle ));
	Game* game = (Game*)handle;
	game->Save();
}


void GameDeviceLoss( void* handle )
{
	CheckThread check;

	GLOUTPUT(( "GameDeviceLoss. handle=%x\n", handle ));
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


void GameHotKey( void* handle, int mask )
{
	CheckThread check;

	Game* game = (Game*)handle;
	return game->HandleHotKey( mask );
}



void PlatformPathToResource( char* buffer, int bufferLen, int* offset, int* length )
{
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
	*offset = 0;
	*length = 0;
#elif defined (ANDROID_NDK)
	grinliz::StrNCpy( buffer, androidResourcePath, bufferLen );
	*offset = androidResourceOffset;
	*length = androidResourceLen;
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


static char prefPath[256] = { 0 };

void GetSystemPath(int root, const char* filename, grinliz::GLString* out)
{
	grinliz::GLString path;

	if (root == GAME_SAVE_DIR) {
		if (!prefPath[0]) {
#ifdef _WIN32
			// The SDL path has a couple of problems on Windows.
			//	1. It doesn't work. The code doesn't actually create the
			//	   directory even though it returns without an error.
			//	   I haven't run the code through a debugger to see why.
			//  2. It creates it in AppData/Roaming. While that makes 
			//	   some sense - it is a standard - it's also a hidden
			//	   directory which is awkward and weird.
			PWSTR pwstr = 0;
			PWSTR append = L"\\AlteraOrbis";
			WCHAR buffer[256];

			SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pwstr);
			PWSTR p = pwstr;
			WCHAR* q = buffer;
			while (*p) {
				*q++ = *p++;
			}
			p = append;
			while( *p ) {
				*q++ = *p++;
			}
			*q = 0;
			CreateDirectory(buffer, NULL);

			// FIXME: should be UTF-8
			char* target = prefPath;
			p = buffer;
			while( *p ) {
				*target = (char)*p;
				target++; p++;
			}
			*target = '\\';
			target++;
			*target = 0;

			CoTaskMemFree(static_cast<void*>(pwstr));
#else
			char* p = SDL_GetPrefPath("GrinningLizard", "AlteraOrbis");
			GLASSERT(p);
			grinliz::StrNCpy(prefPath, p, 256);
			SDL_free(p);
#endif
		}
		out->append(prefPath);
	}
	out->append(filename);
}
