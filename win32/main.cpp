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

#pragma warning ( disable : 4530 )		// Don't warn about unused exceptions.

#include "glew.h"
#include "../libs/SDL2/include/SDL.h"

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glstringutil.h"
#include "../Shiny/include/Shiny.h"

#include "../xegame/cgame.h"
#include "../game/gamesettings.h"

#include "../shared/lodepng.h"

#include "../version.h"
#include "wglew.h"

// For error logging.
#define WINDOWS_LEAN_AND_MEAN
#include <winhttp.h>

//#define TEST_ROTATION
//#define TEST_FULLSPEED
//#define SEND_CRASH_LOGS

#define TIME_BETWEEN_FRAMES	1000/33

static const float KEY_ZOOM_SPEED		= 0.04f;
static const float KEY_ROTATE_SPEED		= 4.0f;
static const float KEY_MOVE_SPEED		= 0.4f;

#if 1
// 4:3 test
static const int SCREEN_WIDTH  = 800;
static const int SCREEN_HEIGHT = 600;
#else
static const int SCREEN_WIDTH  = 952;
static const int SCREEN_HEIGHT = 600;
#endif


const int multisample = 2;
bool fullscreen = false;
int screenWidth  = SCREEN_WIDTH;
int screenHeight = SCREEN_HEIGHT;
bool cameraIso = true;

int nModDB = 0;
grinliz::GLString* databases[GAME_MAX_MOD_DATABASES];	

#ifdef TEST_ROTATION
const int rotation = 1;
#else
const int rotation = 0;
#endif

void ScreenCapture( const char* baseFilename, bool appendCount, bool trim, bool makeTransparent, grinliz::Rectangle2I* size );
void PostCurrentGame();

void TransformXY( int x0, int y0, int* x1, int* y1 )
{
	// As a way to do scaling outside of the core, translate all
	// the mouse coordinates so that they are reported in opengl
	// window coordinates.
	if ( rotation == 0 ) {
		*x1 = x0;
		*y1 = screenHeight-1-y0;
	}
	else if ( rotation == 1 ) {
		*x1 = x0;
		*y1 = screenHeight-1-y0;
	}
	else {
		GLASSERT( 0 );
	}
}


int main( int argc, char **argv )
{    
	MemStartCheck();
	{ char* test = new char[16]; delete [] test; }

	{
		grinliz::GLString releasePath;
		GetSystemPath(GAME_SAVE_DIR, "release_log.txt", &releasePath);
		SetReleaseLog(fopen(releasePath.c_str(), "w"));
	}
	GLOUTPUT_REL(( "Altera startup. version'%s'\n", VERSION ));

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	GLOUTPUT_REL(("SDL version compiled: %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	GLOUTPUT_REL(("SDL version linked:   %d.%d.%d\n", linked.major, linked.minor, linked.patch));
	GLASSERT((linked.major == compiled.major && linked.minor == compiled.minor));

	// SDL initialization steps.
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO ) < 0 )
	{
	    fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError( ) );
		exit( 1 );
	}

	//  OpenGL 4.3 provides full compatibility with OpenGL ES 3.0.
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8);

	if ( multisample ) {
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, multisample );
	}

	if ( argc == 3 ) {
		screenWidth = atoi( argv[1] );
		screenHeight = atoi( argv[2] );
		if ( screenWidth <= 0 ) screenWidth   = SCREEN_WIDTH;
		if ( screenHeight <= 0 ) screenHeight = SCREEN_HEIGHT;
	}

	// Note that our output surface is rotated from the iPod.
	SDL_Window *screen = SDL_CreateWindow(	"Altera",
											50, 50,
											screenWidth, screenHeight,
											/*SDL_WINDOW_FULLSCREEN | */ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
	GLASSERT( screen );
	SDL_GL_CreateContext( screen );

	int stencil = 0;
	int depth = 0;
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &stencil );
	glGetIntegerv( GL_DEPTH_BITS, &depth );
	GLOUTPUT_REL(( "SDL screen created. stencil=%d depthBits=%d\n", 
					stencil, depth ));

    /* Verify there is a surface */
    if ( !screen ) {
	    fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) );
	    exit( 1 );
	}

	int r = glewInit();
	GLASSERT( r == GL_NO_ERROR );

	const unsigned char* vendor   = glGetString( GL_VENDOR );
	const unsigned char* renderer = glGetString( GL_RENDERER );
	const unsigned char* version  = glGetString( GL_VERSION );

	GLOUTPUT_REL(( "OpenGL vendor: '%s'  Renderer: '%s'  Version: '%s'\n", vendor, renderer, version ));

	bool done = false;
	bool zooming = false;
    SDL_Event event;

	float yRotation = 45.0f;
	grinliz::Vector2I mouseDown = { 0, 0 };
	grinliz::Vector2I prevMouseDown = { 0, 0 };
	grinliz::Vector2I rightMouseDown = { -1, -1 };
	U32 prevMouseDownTime = 0;

	int zoomX = 0;
	int zoomY = 0;

	void* game = NewGame( screenWidth, screenHeight, rotation );
	
	int modKeys = SDL_GetModState();
	U32 tickTimer = 0;

	// ---- Main Loop --- //
	while ( !done ) {
		while (SDL_PollEvent(&event)) {

			switch (event.type)
			{
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					screenWidth = event.window.data1;
					screenHeight = event.window.data2;
					GameDeviceLoss(game);
					GameResize(game, screenWidth, screenHeight, rotation);
				}
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.scancode)
				{
				case SDL_SCANCODE_LCTRL:	modKeys = modKeys & (~KMOD_LCTRL);		break;
				case SDL_SCANCODE_RCTRL:	modKeys = modKeys & (~KMOD_RCTRL);		break;
				case SDL_SCANCODE_LSHIFT:	modKeys = modKeys & (~KMOD_LSHIFT);	break;
				case SDL_SCANCODE_RSHIFT:	modKeys = modKeys & (~KMOD_RSHIFT);	break;
				default:
					break;
				}
				break;
			case SDL_KEYDOWN:
				{
					// sym or scancode? I used a dvorak keyboard, so appreciate
					// every day the difference. However, AWSD support is the 
					// primary thing so scancode is hopefully the better choice.
					switch (event.key.keysym.scancode)
					{
					case SDL_SCANCODE_LCTRL:	modKeys = modKeys | KMOD_LCTRL;		break;
					case SDL_SCANCODE_RCTRL:	modKeys = modKeys | KMOD_RCTRL;		break;
					case SDL_SCANCODE_LSHIFT:	modKeys = modKeys | KMOD_LSHIFT;	break;
					case SDL_SCANCODE_RSHIFT:	modKeys = modKeys | KMOD_RSHIFT;	break;

					case SDL_SCANCODE_ESCAPE:
						{
		#ifdef DEBUG
							// only escape out in debug mode
							// if ( !handled ) 
							done = true;
		#endif
						}
						break;

					case SDL_SCANCODE_F4:
						{
							int sdlMod = SDL_GetModState();
							if (sdlMod & (KMOD_RALT | KMOD_LALT))
								done = true;
						}
						break;

					case SDL_SCANCODE_SPACE:GameHotKey(game, GAME_HK_SPACE);			break;
					case SDL_SCANCODE_F1:	GameHotKey(game, GAME_HK_TOGGLE_DEBUG_TEXT);	break;
					case SDL_SCANCODE_F2:	GameHotKey(game, GAME_HK_TOGGLE_DEBUG_UI);		break;
					// F3: screenshot

					//case SDLK_a: reserved
					case SDL_SCANCODE_B:	GameHotKey(game, GAME_HK_CHEAT_GOLD);		break;
					case SDL_SCANCODE_C:	GameHotKey(game, GAME_HK_TOGGLE_COLORS);	break;
					//case SDLK_d: reserved
					case SDL_SCANCODE_E:	GameHotKey(game, GAME_HK_CHEAT_ELIXIR);	break;
					case SDL_SCANCODE_F:	GameHotKey(game, GAME_HK_TOGGLE_FAST);	break;
					case SDL_SCANCODE_H:	GameHotKey(game, GAME_HK_TOGGLE_PATHING);	break;
					case SDL_SCANCODE_K:	GameHotKey(game, GAME_HK_CHEAT_CRYSTAL);	break;
					case SDL_SCANCODE_M:	GameHotKey(game, GAME_HK_MAP);			break;
					case SDL_SCANCODE_P:	GameHotKey(game, GAME_HK_TOGGLE_PERF);	break;
					case SDL_SCANCODE_Q:	GameHotKey(game, GAME_HK_CHEAT_HERD);	break;
					//case SDLK_s: reserved
					case SDL_SCANCODE_T:	GameHotKey(game, GAME_HK_CHEAT_TECH);		break;
					case SDL_SCANCODE_U:	GameHotKey(game, GAME_HK_TOGGLE_UI);		break;
					//case SDLK_w: reserved

					case SDL_SCANCODE_1:	GameHotKey(game, GAME_HK_TOGGLE_GLOW);	break;
					case SDL_SCANCODE_2:	GameHotKey(game, GAME_HK_TOGGLE_PARTICLE);	break;
					case SDL_SCANCODE_3:	GameHotKey(game, GAME_HK_TOGGLE_VOXEL);	break;
					case SDL_SCANCODE_4:	GameHotKey(game, GAME_HK_TOGGLE_SHADOW);	break;
					case SDL_SCANCODE_5:	GameHotKey(game, GAME_HK_TOGGLE_BOLT);	break;

					case SDL_SCANCODE_F3:
						GameDoTick(game, SDL_GetTicks());
						SDL_GL_SwapWindow(screen);
						ScreenCapture("cap", true, false, false, 0);
						break;

					default:
						break;
					}
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				{
					int x, y;
					TransformXY(event.button.x, event.button.y, &x, &y);
					GLOUTPUT(("Mouse down %d %d\n", x, y));

					int mod = 0;
					if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
					else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;

					if (event.button.button == SDL_BUTTON_LEFT) {
						mouseDown.Set(event.button.x, event.button.y);
						GameTap(game, GAME_TAP_DOWN, x, y, mod);
					}
					else if (event.button.button == SDL_BUTTON_RIGHT) {
						GameTap(game, GAME_TAP_CANCEL, x, y, mod);
						rightMouseDown.Set(-1, -1);
						if (mod == 0) {
							rightMouseDown.Set(event.button.x, event.button.y);
							GameCameraPan(game, GAME_PAN_START, float(x), float(y));
						}
						else if (mod == GAME_TAP_MOD_CTRL) {
							zooming = true;
							//GameCameraRotate( game, GAME_ROTATE_START, 0.0f );
							SDL_GetRelativeMouseState(&zoomX, &zoomY);
						}
					}
				}
				break;

			case SDL_MOUSEBUTTONUP:
				{
					int x, y;
					TransformXY(event.button.x, event.button.y, &x, &y);

					if (event.button.button == 3) {
						zooming = false;
						if (rightMouseDown.x >= 0 && rightMouseDown.y >= 0) {
							GameCameraPan(game, GAME_PAN_END, float(x), float(y));
							rightMouseDown.Set(-1, -1);
						}
					}
					if (event.button.button == 1) {
						int mod = 0;
						if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
						else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;
						GameTap(game, GAME_TAP_UP, x, y, mod);
					}
				}
				break;

			case SDL_MOUSEMOTION:
				{
					SDL_GetRelativeMouseState(&zoomX, &zoomY);
					int state = SDL_GetMouseState(NULL, NULL);
					int x, y;
					TransformXY(event.button.x, event.button.y, &x, &y);

					int mod = 0;
					if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
					else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;

					if (state & SDL_BUTTON(1)) {
						GameTap(game, GAME_TAP_MOVE, x, y, mod);
					}
					else if (rightMouseDown.x >= 0 && rightMouseDown.y >= 0) {
						GameCameraPan(game, GAME_PAN_END, float(x), float(y));
					}
					else if (zooming && (state & SDL_BUTTON(3))) {
						float deltaZoom = 0.01f * (float)zoomY;
						GameZoom(game, GAME_ZOOM_DISTANCE, deltaZoom);
						GameCameraRotate(game, (float)(zoomX)*0.5f);
					}
					else if (((state & SDL_BUTTON(1)) == 0)) {
						GameTap(game, GAME_MOVE_WHILE_UP, x, y, mod);
					}
				}
				break;

			case SDL_QUIT:
				{
					done = true;
				}
				break;

			default:
				break;
			}
		}
		U32 delta = SDL_GetTicks() - tickTimer;
		if ( delta < TIME_BETWEEN_FRAMES ) {
			SDL_Delay(1);
			continue;
		}
		tickTimer = SDL_GetTicks();
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );

		const U8* keys = SDL_GetKeyboardState(0);
		if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S] ) {
			if (modKeys & KMOD_CTRL)
				GameZoom(game, GAME_ZOOM_DISTANCE, KEY_ZOOM_SPEED);
			else
				GameCameraMove(game, 0, -KEY_MOVE_SPEED);
		}
		if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) {
			if (modKeys & KMOD_CTRL)
				GameZoom(game, GAME_ZOOM_DISTANCE, -KEY_ZOOM_SPEED);
			else
				GameCameraMove(game, 0, KEY_MOVE_SPEED);
		}
		if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
			if (modKeys & KMOD_CTRL)
				GameCameraRotate(game, KEY_ROTATE_SPEED);
			else
				GameCameraMove(game, -KEY_MOVE_SPEED, 0);
		}
		if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
			if (modKeys & KMOD_CTRL)
				GameCameraRotate(game, -KEY_ROTATE_SPEED);
			else
				GameCameraMove(game, KEY_MOVE_SPEED, 0);
		}

		{
			PROFILE_BLOCK( GameDoTick );
			GameDoTick( game, SDL_GetTicks() );
		}
		{
			PROFILE_BLOCK( Swap );
			SDL_GL_SwapWindow( screen );
		}
	}

	GameSave( game );
	DeleteGame( game );

	for( int i=0; i<nModDB; ++i ) {
		delete databases[i];
	}

	SDL_Quit();

#if SEND_CRASH_LOGS
	// Empty the file - quit went just fine.
	{
		FILE* fp = FOpen( "UFO_Running.txt", "w" );
		if ( fp )
			fclose( fp );
	}
#endif

	MemLeakCheck();
	return 0;
}


// 0:lowX, 1:highX, 2:lowY, 3:highY
bool RectangleIsBlack( const grinliz::Rectangle2I& r, int edge, SDL_Surface* surface )
{
	grinliz::Rectangle2I c;
	switch( edge ) {
	case 0: c.Set( r.min.x, r.min.y, r.min.x, r.max.y ); break;
	case 1: c.Set( r.max.x, r.min.y, r.max.x, r.max.y ); break;
	case 2: c.Set( r.min.x, r.min.y, r.max.x, r.min.y ); break;
	case 3: c.Set( r.min.x, r.max.y, r.max.x, r.max.y ); break;
	}

	for( int y=c.min.y; y<=c.max.y; ++y ) {
		for( int x=c.min.x; x<=c.max.x; ++x ) {
			U32 color = *( (U32*)surface->pixels + y*surface->w + x ) & 0x00ffffff;
			if ( color )
				return false;
		}
	}
	return true;
}


void ScreenCapture( const char* baseFilename, bool appendCount, bool trim, bool makeTransparent, grinliz::Rectangle2I* size )
{
	int viewPort[4];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	int width  = viewPort[2]-viewPort[0];
	int height = viewPort[3]-viewPort[1];

	SDL_Surface* surface = SDL_CreateRGBSurface( 0, width, height, 32, 0xff, 0xff<<8, 0xff<<16, 0xff<<24 );
	if ( !surface )
		return;

	glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels );

	// This is a fancy swap, for the screen pixels:
	int i;
	U32* buffer = new U32[width];

	for( i=0; i<height/2; ++i )
	{
		memcpy( buffer, 
				( (U32*)surface->pixels + i*width ), 
				width*4 );
		memcpy( ( (U32*)surface->pixels + i*width ), 
				( (U32*)surface->pixels + (height-1-i)*width ),
				width*4 );
		memcpy( ( (U32*)surface->pixels + (height-1-i)*width ),
				buffer,
				width*4 );
	}
	delete [] buffer;

	// And now, set all the alphas to opaque:
	for( i=0; i<width*height; ++i )
		*( (U32*)surface->pixels + i ) |= 0xff000000;

	grinliz::Rectangle2I r;
	r.Set( 0, 0, width-1, height-1 );
	if ( trim ) {

		while ( r.min.x < r.max.x && RectangleIsBlack( r, 0, surface )) {
			++r.min.x;
		}
		while ( r.min.x < r.max.x && RectangleIsBlack( r, 1, surface )) {
			--r.max.x;
		}
		while ( r.min.y < r.max.y && RectangleIsBlack( r, 2, surface )) {
			++r.min.y;
		}
		while ( r.min.y < r.max.y && RectangleIsBlack( r, 3, surface )) {
			--r.max.y;
		}
		SDL_Surface* newSurface = SDL_CreateRGBSurface( 0, r.Width(), r.Height(), 32, 0xff, 0xff<<8, 0xff<<16, 0xff<<24 );

		// Stupid BLT semantics consider dst alpha.
		memset( newSurface->pixels, 255, newSurface->pitch*r.Height() );
		SDL_Rect srcRect = { r.min.x, r.min.y, r.Width(), r.Height() };
		SDL_Rect dstRect = { 0, 0, r.Width(), r.Height() };
		SDL_BlitSurface( surface, &srcRect, newSurface, &dstRect ); 

		SDL_FreeSurface( surface );
		surface = newSurface;
		width = r.Width();
		height = r.Height();
	}
	if ( size ) {
		*size = r;
	}
	if ( width == 0 || height == 0 ) {
		return;
	}

	if ( makeTransparent ) {
		// And now, set all the alphas to opaque:
		for( i=0; i<width*height; ++i ) {
			if (( *( (U32*)surface->pixels + i ) & 0x00ffffff ) == 0 ) {
				*( (U32*)surface->pixels + i ) = 0;
			}
		}
	}

	int index = 0;
	char buf[ 256 ];
	if ( appendCount ) {
		for( index = 0; index<100; ++index )
		{
			grinliz::SNPrintf( buf, 256, "%s%02d.png", baseFilename, index );
			grinliz::GLString path;
			GetSystemPath(GAME_SAVE_DIR, buf, &path);
			FILE* fp = fopen(path.c_str(), "rb" );
			if ( fp )
				fclose( fp );
			else
				break;
		}
	}
	else {
		grinliz::SNPrintf( buf, 256, "%s.png", baseFilename );
	}
	if ( index < 100 ) {
//		SDL_SaveBMP( surface, buf );
		lodepng_encode32_file( buf, (const unsigned char*)surface->pixels, width, height );
	}
	SDL_FreeSurface(surface); 
}
