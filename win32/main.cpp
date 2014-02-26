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
#include "audio.h"
#include "wglew.h"

// For error logging.
#define WINDOWS_LEAN_AND_MEAN
#include <winhttp.h>

//#define TEST_ROTATION
//#define TEST_FULLSPEED
//#define SEND_CRASH_LOGS
#define USE_LOOP

#define TIME_BETWEEN_FRAMES	1000/33

static const float KEY_ZOOM_SPEED		= 0.02f;
static const float KEY_ROTATE_SPEED		= 2.0f;

#if 1
// A default screenshot size for market.
static const int SCREEN_WIDTH  = 800;
static const int SCREEN_HEIGHT = 600;
#endif
#if 0
// 4:3 test
static const int SCREEN_WIDTH  = 800;
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


#ifndef USE_LOOP
Uint32 TimerCallback(Uint32 interval, void *param)
{
	SDL_Event user;
	memset( &user, 0, sizeof(user ) );
	user.type = SDL_USEREVENT;

	SDL_PushEvent( &user );
	return interval;
}
#endif


int main( int argc, char **argv )
{    
	MemStartCheck();
	{ char* test = new char[16]; delete [] test; }

	GLOUTPUT_REL(( "Altera startup. version=%d\n", VERSION ));

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
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

	Audio_Init();

	bool done = false;
	bool zooming = false;
    SDL_Event event;

	float yRotation = 45.0f;
	grinliz::Vector2I mouseDown = { 0, 0 };
	grinliz::Vector2I prevMouseDown = { 0, 0 };
	U32 prevMouseDownTime = 0;

	int zoomX = 0;
	int zoomY = 0;

	void* game = NewGame( screenWidth, screenHeight, rotation, ".\\" );
	
#ifndef TEST_FULLSPEED
#ifndef USE_LOOP
	SDL_TimerID timerID = SDL_AddTimer( TIME_BETWEEN_FRAMES, TimerCallback, 0 );
#endif
#endif

	int modKeys = SDL_GetModState();
#ifdef USE_LOOP
	U32 tickTimer = 0;
#endif

	// ---- Main Loop --- //
#if defined(TEST_FULLSPEED) || defined(USE_LOOP)
	while ( !done ) {
		while ( SDL_PollEvent( &event ) )
#else
	while ( !done && SDL_WaitEvent( &event ) )
	{
		// The user event shouldn't be duplicated...if there are 2, pull out the dupe.
		if ( event.type == SDL_USEREVENT ) {
			SDL_Event e;
			while( true ) {
				int n = SDL_PeepEvents( &e, 1, SDL_PEEKEVENT, SDL_ALLEVENTS );		
				if ( n == 1 && e.type == SDL_USEREVENT ) {
					SDL_PeepEvents( &e, 1, SDL_GETEVENT, SDL_ALLEVENTS );
				}
				else {
					break;
				}
			}
		}
#endif

		switch( event.type )
		{
			case SDL_WINDOWEVENT:
				if ( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
					screenWidth  = event.window.data1;
					screenHeight = event.window.data2;
					GameDeviceLoss( game );
					GameResize( game, screenWidth, screenHeight, rotation );
				}
				break;

			case SDL_KEYUP:
				switch ( event.key.keysym.sym )
				{
					case SDLK_LCTRL:	modKeys = modKeys & (~KMOD_LCTRL );		break;
					case SDLK_RCTRL:	modKeys = modKeys & (~KMOD_RCTRL );		break;
					case SDLK_LSHIFT:	modKeys = modKeys & (~KMOD_LSHIFT );	break;
					case SDLK_RSHIFT:	modKeys = modKeys & (~KMOD_RSHIFT );	break;
					default:
						break;
				}
				break;
			case SDL_KEYDOWN:
			{
				switch ( event.key.keysym.sym )
				{
					case SDLK_LCTRL:	modKeys = modKeys | KMOD_LCTRL;		break;
					case SDLK_RCTRL:	modKeys = modKeys | KMOD_RCTRL;		break;
					case SDLK_LSHIFT:	modKeys = modKeys | KMOD_LSHIFT;	break;
					case SDLK_RSHIFT:	modKeys = modKeys | KMOD_RSHIFT;	break;

					case SDLK_ESCAPE:
						{
#ifdef DEBUG
							// only escape out in debug mode
							// if ( !handled ) 
							done = true;
#endif
						}
						break;

					case SDLK_F4:
						{
							int sdlMod = SDL_GetModState();
							if ( sdlMod & ( KMOD_RALT | KMOD_LALT ) )
								done = true;
						}
						break;

					case SDLK_SPACE:GameHotKey( game, GAME_HK_SPACE );			break;
					case SDLK_a:	GameHotKey( game, GAME_HK_TOGGLE_PATHING );	break;
					case SDLK_b:	GameHotKey( game, GAME_HK_CHEAT_GOLD );		break;
					case SDLK_c:	GameHotKey( game, GAME_HK_TOGGLE_COLORS );	break;
					case SDLK_d:	GameHotKey( game, GAME_HK_TOGGLE_DEBUG_TEXT );	break;
					case SDLK_f:	GameHotKey( game, GAME_HK_TOGGLE_FAST );	break;
					case SDLK_i:	GameHotKey( game, GAME_HK_TOGGLE_DEBUG_UI );		break;
					case SDLK_k:	GameHotKey( game, GAME_HK_CHEAT_CRYSTAL );	break;
					case SDLK_m:	GameHotKey( game, GAME_HK_MAP );			break;
					case SDLK_p:	GameHotKey( game, GAME_HK_TOGGLE_PERF );	break;
					case SDLK_q:	GameHotKey( game, GAME_HK_CHEAT_HERD );	break;
					case SDLK_t:	GameHotKey( game, GAME_HK_CHEAT_TECH );		break;
					case SDLK_u:	GameHotKey( game, GAME_HK_TOGGLE_UI );		break;

					case SDLK_1:	GameHotKey( game, GAME_HK_TOGGLE_GLOW );	break;
					case SDLK_2:	GameHotKey( game, GAME_HK_TOGGLE_PARTICLE );	break;
					case SDLK_3:	GameHotKey( game, GAME_HK_TOGGLE_VOXEL );	break;
					case SDLK_4:	GameHotKey( game, GAME_HK_TOGGLE_SHADOW );	break;
					case SDLK_5:	GameHotKey( game, GAME_HK_TOGGLE_BOLT );	break;

					case SDLK_s:
						GameDoTick( game, SDL_GetTicks() );
						SDL_GL_SwapWindow( screen );
						ScreenCapture( "cap", true, false, false, 0 );
						break;

					default:
						break;
				}
/*					GLOUTPUT(( "fov=%.1f rot=%.1f h=%.1f\n", 
							game->engine.fov, 
							game->engine.camera.Tilt(), 
							game->engine.camera.PosWC().y ));
*/
			}
			break;

			case SDL_MOUSEBUTTONDOWN:
			{
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );
				GLOUTPUT(( "Mouse down %d %d\n", x, y ));

				mouseDown.Set( event.button.x, event.button.y );

				int mod=0;
				if ( modKeys & ( KMOD_LSHIFT | KMOD_RSHIFT ))    mod = GAME_TAP_MOD_SHIFT;
				else if ( modKeys & ( KMOD_LCTRL | KMOD_RCTRL )) mod = GAME_TAP_MOD_CTRL;

				if ( event.button.button == 1 ) {
					GameTap( game, GAME_TAP_DOWN, x, y, mod );
				}
				else if ( event.button.button == 3 ) {
					GameTap( game, GAME_TAP_CANCEL, x, y, mod );
					zooming = true;
					//GameCameraRotate( game, GAME_ROTATE_START, 0.0f );
					SDL_GetRelativeMouseState( &zoomX, &zoomY );
				}
			}
			break;

			case SDL_MOUSEBUTTONUP:
			{
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );

				if ( event.button.button == 3 ) {
					zooming = false;
				}
				if ( event.button.button == 1 ) {
					int mod = 0;
					if ( modKeys & ( KMOD_LSHIFT | KMOD_RSHIFT ))    mod = GAME_TAP_MOD_SHIFT;
					else if ( modKeys & ( KMOD_LCTRL | KMOD_RCTRL )) mod = GAME_TAP_MOD_CTRL;
					GameTap( game, GAME_TAP_UP, x, y, mod );
				}
			}
			break;

			case SDL_MOUSEMOTION:
			{
				SDL_GetRelativeMouseState( &zoomX, &zoomY );
				int state = SDL_GetMouseState(NULL, NULL);
				int x, y;
				TransformXY( event.button.x, event.button.y, &x, &y );

				int mod = 0;
				if ( modKeys & ( KMOD_LSHIFT | KMOD_RSHIFT ))    mod = GAME_TAP_MOD_SHIFT;
				else if ( modKeys & ( KMOD_LCTRL | KMOD_RCTRL )) mod = GAME_TAP_MOD_CTRL;

				if ( state & SDL_BUTTON(1) ) {
					GameTap( game, GAME_TAP_MOVE, x, y, mod );
				}
				else if ( zooming && (state & SDL_BUTTON(3)) ) {
					float deltaZoom = 0.01f * (float)zoomY;
					GameZoom( game, GAME_ZOOM_DISTANCE, deltaZoom );
					GameCameraRotate( game, (float)(zoomX)*0.5f );
				}
				else if ( ( ( state & SDL_BUTTON(1) ) == 0 ) ) {
					GameTap( game, GAME_TAP_MOVE_UP, x, y, mod );
				}
			}
			break;

			case SDL_QUIT:
			{
				done = true;
			}
			break;

			case SDL_USEREVENT:
			{
				glEnable( GL_DEPTH_TEST );
				glDepthFunc( GL_LEQUAL );

				const U8* keys =  SDL_GetKeyboardState( 0 );
				if ( keys[SDL_SCANCODE_PAGEDOWN] ) {
					GameZoom( game, GAME_ZOOM_DISTANCE, KEY_ZOOM_SPEED );
				}
				if ( keys[SDL_SCANCODE_PAGEUP] ) {
					GameZoom( game, GAME_ZOOM_DISTANCE, -KEY_ZOOM_SPEED );
				}
				if ( keys[SDL_SCANCODE_HOME] ) {
					GameCameraRotate( game, -KEY_ROTATE_SPEED );
				}
				if ( keys[SDL_SCANCODE_END] ) {
					GameCameraRotate( game, KEY_ROTATE_SPEED );
				}

				// This only works fullscreen (not just full window) which
				// isn't supported yet.
				/*
				int x, y;
				int buttons = SDL_GetMouseState( &x, &y );
				if ( buttons == 0 ) {
					if ( x <= 0 )
						GameCameraRotate( game, -KEY_ROTATE_SPEED );
					else if ( x >= surface->w-1 )
						GameCameraRotate( game, -KEY_ROTATE_SPEED );
				}
				*/
				GameDoTick( game, SDL_GetTicks() );
				SDL_GL_SwapWindow( screen );

				int databaseID=0, size=0, offset=0;
				// FIXME: account for databaseID when looking up sound.
				while ( GamePopSound( game, &databaseID, &offset, &size ) ) {
					Audio_PlayWav( "./res/uforesource.db", offset, size );
				}
			};

			default:
				break;
		}
#if defined(TEST_FULLSPEED) || defined(USE_LOOP)

#ifdef USE_LOOP
		U32 delta = SDL_GetTicks() - tickTimer;
		if ( delta < TIME_BETWEEN_FRAMES ) {
			SDL_Delay(0);
			continue;
		}
		tickTimer = SDL_GetTicks();
#endif
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );

		const U8* keys =  SDL_GetKeyboardState( 0 );
		if ( keys[SDL_SCANCODE_PAGEDOWN] ) {
			GameZoom( game, GAME_ZOOM_DISTANCE, KEY_ZOOM_SPEED );
		}
		if ( keys[SDL_SCANCODE_PAGEUP] ) {
			GameZoom( game, GAME_ZOOM_DISTANCE, -KEY_ZOOM_SPEED );
		}
		if ( keys[SDL_SCANCODE_HOME] ) {
			GameCameraRotate( game, -KEY_ROTATE_SPEED );
		}
		if ( keys[SDL_SCANCODE_END] ) {
			GameCameraRotate( game, KEY_ROTATE_SPEED );
		}

		{
			PROFILE_BLOCK( GameDoTick );
			GameDoTick( game, SDL_GetTicks() );
		}
		{
			PROFILE_BLOCK( Swap );
			SDL_GL_SwapWindow( screen );
		}

#ifdef USE_LOOP
		int databaseID=0, size=0, offset=0;
		// FIXME: account for databaseID when looking up sound.
		while ( GamePopSound( game, &databaseID, &offset, &size ) ) {
			Audio_PlayWav( "./res/uforesource.db", offset, size );
		}
#endif

	}
#else
	}
	SDL_RemoveTimer( timerID );
#endif

	// FIXME: turn save on exit back on.
	//GameSave( game );
	DeleteGame( game );
	Audio_Close();

	for( int i=0; i<nModDB; ++i ) {
		delete databases[i];
	}

	SDL_Quit();

#if SEND_CRASH_LOGS
	// Empty the file - quit went just fine.
	{
		FILE* fp = fopen( "UFO_Running.txt", "w" );
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
	#pragma warning ( push )
	#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
			FILE* fp = fopen( buf, "rb" );
	#pragma warning ( pop )
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
