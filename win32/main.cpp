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
#include "../grinliz/glcontainer.h"
#include "../Shiny/include/Shiny.h"

#include "../xegame/cgame.h"
#include "../game/gamesettings.h"
#include "../engine/platformgl.h"

#include "../shared/lodepng.h"

#include "../version.h"
#if defined (UFO_WIN32_SDL)
#include "wglew.h"
#else
#include "glew.h"
#endif

//#define TEST_ROTATION
//#define TEST_FULLSPEED
//#define SEND_CRASH_LOGS
//#define OUTPUT_MOUSE_AND_TOUCH

#define TIME_BETWEEN_FRAMES	1000/33

static const float KEY_ZOOM_SPEED		= 0.04f;
static const float KEY_ROTATE_SPEED		= 4.0f;
static const float KEY_MOVE_SPEED		= 0.4f;

// 4:3 test			800x600
// virtual size		800x640	virtual: 750x600

// Laptop
static const int SCREEN_WIDTH  = 700;
static const int SCREEN_HEIGHT = 520;

const int multisample = 2;
bool fullscreen = false;
int restoreWidth = SCREEN_WIDTH;
int restoreHeight = SCREEN_HEIGHT;
bool cameraIso = true;


int nModDB = 0;
grinliz::GLString* databases[GAME_MAX_MOD_DATABASES];	

void ScreenCapture();
void PostCurrentGame();

bool PlatformHasMouseSupport() {
	return true;
}


int main(int argc, char **argv)
{
	MemStartCheck();
	{ char* test = new char[16]; delete[] test; }
	grinliz::TestContainers();

	{
		grinliz::GLString releasePath;
		GetSystemPath(GAME_SAVE_DIR, "release_log.txt", &releasePath);
		SetReleaseLog(fopen(releasePath.c_str(), "w"));
	}
	GLOUTPUT_REL(("Altera startup. version'%s'\n", VERSION));

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	GLOUTPUT_REL(("SDL version compiled: %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	GLOUTPUT_REL(("SDL version linked:   %d.%d.%d\n", linked.major, linked.minor, linked.patch));
	GLASSERT((linked.major == compiled.major && linked.minor == compiled.minor));

	// SDL initialization steps.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0)
	{
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		exit(1);
	}

	//  OpenGL 4.3 provides full compatibility with OpenGL ES 3.0.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef DEBUG
#if 0	// I was hoping to get to Angle on intel - may still be able to. But
	// as is this gets HW mode, which crashes in a function that should
	// be supported. The Intel drivers are so terrible. As of this writing,
	// you can't specify the DX version of ES:
	//		http://forums.libsdl.org/viewtopic.php?t=9770&highlight=angle+opengl
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);	// driver supports 2 and 3. Both crash.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#if 0
	// The trickier 3.2 context.
	// No GL_QUADs anymore.
	// All the attributes need to be floats.
	// No ALPHA textures.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
#if 0
	// In theory the minimum supported version:
	// has instancing, and modern shader syntax
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
#endif

	if (multisample) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample);
	}

	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);

	int screenX = displayMode.w / 8;
	int screenY = displayMode.h / 8;
	int screenWidth = displayMode.w * 3 / 4;
	int screenHeight = displayMode.h * 3 / 4;

	if (argc == 3) {
		screenWidth = atoi(argv[1]);
		screenHeight = atoi(argv[2]);
		if (screenWidth <= 0) screenWidth = SCREEN_WIDTH;
		if (screenHeight <= 0) screenHeight = SCREEN_HEIGHT;
	}

	restoreWidth = screenWidth;
	restoreHeight = screenHeight;

	SDL_Window *screen = SDL_CreateWindow("Altera",
		screenX, screenY, screenWidth, screenHeight,
		/*SDL_WINDOW_FULLSCREEN | */ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLASSERT(screen);
	SDL_GL_CreateContext(screen);

	int stencil = 0;
	int depth = 0;
	CHECK_GL_ERROR;
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
	glGetError();	// the above stencil/depth query sometimes does throw an error.
	glGetError();	// 2 queries, 2 errors.
	CHECK_GL_ERROR;
	GLOUTPUT_REL(("SDL screen created. stencil=%d depthBits=%d\n", stencil, depth));

	/* Verify there is a surface */
	if (!screen) {
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
		exit(1);
	}

	CHECK_GL_ERROR;
	glewExperimental = GL_TRUE;
	int r = glewInit();
	GLASSERT(r == GL_NO_ERROR);

	while (glGetError() != GL_NO_ERROR) {
		// around again
	}
	CHECK_GL_ERROR;

	const unsigned char* vendor = glGetString(GL_VENDOR);
	const unsigned char* renderer = glGetString(GL_RENDERER);
	const unsigned char* version = glGetString(GL_VERSION);

	GLOUTPUT_REL(("OpenGL vendor: '%s'  Renderer: '%s'  Version: '%s'\n", vendor, renderer, version));
	CHECK_GL_ERROR;

	bool done = false;
	bool zooming = false;
	SDL_Event event;

//	float yRotation = 45.0f;
	grinliz::Vector2I mouseDown = { 0, 0 };
//	grinliz::Vector2I prevMouseDown = { 0, 0 };
	grinliz::Vector2I rightMouseDown = { 0, 0 };
//	U32 prevMouseDownTime = 0;

	int zoomX = 0;
	int zoomY = 0;
	// Used to compute fingers close, but problems:
	// - really to the OS to do that, because of all the tuning
	// - the coordinates are in windows normalized, so can't get the physical distance reliably.
	//bool fingersClose = true;
	int nFingers = 0;

	void* game = NewGame(screenWidth, screenHeight, 0);

	int modKeys = SDL_GetModState();
	U32 tickTimer = 0, lastTick = 0, thisTick = 0;

#ifdef OUTPUT_MOUSE_AND_TOUCH
	int value = GetSystemMetrics(SM_DIGITIZER);
	if (value & NID_INTEGRATED_TOUCH) GLOUTPUT(("NID_INTEGRATED_TOUCH\n"));
	if (value & NID_MULTI_INPUT) GLOUTPUT(("NID_MULTI_INPUT\n"));
	if (value & NID_READY) GLOUTPUT(("NID_READY\n"));
#endif

	grinliz::Vector2F multiTouchStart = { 0, 0 };
//	int mouseMoveCount = 0;

	// ---- Main Loop --- //
	while (!done) {
		while (SDL_PollEvent(&event)) {

			switch (event.type)
			{
				case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					screenWidth = event.window.data1;
					screenHeight = event.window.data2;
					GameDeviceLoss(game);
					GameResize(game, screenWidth, screenHeight, 0);
				}
				break;

				case SDL_KEYUP:
				switch (event.key.keysym.scancode)
				{
					case SDL_SCANCODE_LCTRL:	modKeys = modKeys & (~KMOD_LCTRL);		break;
					case SDL_SCANCODE_RCTRL:	modKeys = modKeys & (~KMOD_RCTRL);		break;
					case SDL_SCANCODE_LSHIFT:	modKeys = modKeys & (~KMOD_LSHIFT);		break;
					case SDL_SCANCODE_RSHIFT:	modKeys = modKeys & (~KMOD_RSHIFT);		break;
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

						case SDL_SCANCODE_F4:
						{
							int sdlMod = SDL_GetModState();
							if (sdlMod & (KMOD_RALT | KMOD_LALT))
								done = true;
						}
						break;

						case SDL_SCANCODE_ESCAPE:	GameHotKey(game, GAME_HK_ESCAPE);			break;
						case SDL_SCANCODE_SPACE:	GameHotKey(game, GAME_HK_TOGGLE_PAUSE);		break;
						case SDL_SCANCODE_RETURN:	GameHotKey(game, GAME_HK_DEBUG_ACTION);		break;
						case SDL_SCANCODE_F1:	GameHotKey(game, GAME_HK_TOGGLE_DEBUG_TEXT);	break;
						case SDL_SCANCODE_F2:	GameHotKey(game, GAME_HK_TOGGLE_DEBUG_UI);		break;
							// F3: screenshot
						case SDL_SCANCODE_TAB:	GameHotKey(game, GAME_HK_CAMERA_TOGGLE);		break;
						case SDL_SCANCODE_HOME:	GameHotKey(game, GAME_HK_CAMERA_CORE);			break;
						case SDL_SCANCODE_END:	GameHotKey(game, GAME_HK_CAMERA_AVATAR);		break;

							//case SDLK_a: reserved
						case SDL_SCANCODE_B:	GameHotKey(game, GAME_HK_CHEAT_GOLD);			break;
						case SDL_SCANCODE_C:	GameHotKey(game, GAME_HK_ATTACH_CORE);			break;
							//case SDLK_d: reserved
						case SDL_SCANCODE_E:	GameHotKey(game, GAME_HK_CHEAT_ELIXIR);			break;
						case SDL_SCANCODE_H:	GameHotKey(game, GAME_HK_TOGGLE_PATHING);		break;
						case SDL_SCANCODE_K:	GameHotKey(game, GAME_HK_CHEAT_CRYSTAL);		break;
						case SDL_SCANCODE_M:	GameHotKey(game, GAME_HK_MAP);					break;
						case SDL_SCANCODE_P:	GameHotKey(game, GAME_HK_TOGGLE_PERF);			break;
						case SDL_SCANCODE_Q:	GameHotKey(game, GAME_HK_CHEAT_HERD);			break;
							//case SDLK_s: reserved
						case SDL_SCANCODE_T:	GameHotKey(game, GAME_HK_CHEAT_TECH);			break;
						case SDL_SCANCODE_U:	GameHotKey(game, GAME_HK_TOGGLE_UI);			break;
							//case SDLK_w: reserved

						case SDL_SCANCODE_1:	GameHotKey(game, GAME_HK_TOGGLE_GLOW);			break;
						case SDL_SCANCODE_2:	GameHotKey(game, GAME_HK_TOGGLE_PARTICLE);		break;
						case SDL_SCANCODE_3:	GameHotKey(game, GAME_HK_TOGGLE_VOXEL);			break;
						case SDL_SCANCODE_4:	GameHotKey(game, GAME_HK_TOGGLE_SHADOW);		break;
						case SDL_SCANCODE_5:	GameHotKey(game, GAME_HK_TOGGLE_BOLT);			break;

						case SDL_SCANCODE_F3:
						GameDoTick(game, SDL_GetTicks());
						SDL_GL_SwapWindow(screen);
						ScreenCapture();
						break;

						case SDL_SCANCODE_F11:
						{
							if (fullscreen) {
								// SDL_RestoreWindow doesn't seem to work as I expect.
								SDL_SetWindowFullscreen(screen, 0);
								SDL_SetWindowSize(screen, restoreWidth, restoreHeight);
								fullscreen = false;
							}
							else {
								restoreWidth = screenWidth;
								restoreHeight = screenHeight;
								SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
								fullscreen = true;
							}
						}
						break;

						default:
						break;
					}
				}
				break;

				case SDL_MOUSEBUTTONDOWN:
				{
					int x = event.button.x;
					int y = event.button.y;

					int mod = 0;
					if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
					else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;

					if (nFingers > 1) {
						// do nothing
					}
					else if (event.button.button == SDL_BUTTON_LEFT) {
						#ifdef OUTPUT_MOUSE_AND_TOUCH
						GLOUTPUT(("Left mouse down %d %d\n", x, y));
						#endif
						mouseDown.Set(event.button.x, event.button.y);
						GameTap(game, GAME_TAP_DOWN, x, y, mod);
					}
					else if (event.button.button == SDL_BUTTON_RIGHT) {
						#ifdef OUTPUT_MOUSE_AND_TOUCH
						GLOUTPUT(("Right mouse down %d %d\n", x, y));
						#endif
						GameTap(game, GAME_TAP_CANCEL, x, y, mod);
						rightMouseDown.Zero();
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
					int x = event.button.x;
					int y = event.button.y;

					if (event.button.button == SDL_BUTTON_RIGHT) {
						#ifdef OUTPUT_MOUSE_AND_TOUCH
						GLOUTPUT(("Right mouse up %d %d\n", x, y));
						#endif
						zooming = false;
						if (!rightMouseDown.IsZero()) {
							GameCameraPan(game, GAME_PAN_END, float(x), float(y));
							rightMouseDown.Zero();
						}
					}
					if (event.button.button == SDL_BUTTON_LEFT) {
						if (!mouseDown.IsZero()) {	// filter out mouse events that become finger events.
							#ifdef OUTPUT_MOUSE_AND_TOUCH
							GLOUTPUT(("Left mouse up %d %d\n", x, y));
							#endif
							int mod = 0;
							if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
							else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;
							GameTap(game, GAME_TAP_UP, x, y, mod);
						}
					}
				}
				break;

				case SDL_MOUSEMOTION:
				{
					SDL_GetRelativeMouseState(&zoomX, &zoomY);
					int state = event.motion.state;
					int x = event.motion.x;
					int y = event.motion.y;

					int mod = 0;
					if (modKeys & (KMOD_LSHIFT | KMOD_RSHIFT))    mod = GAME_TAP_MOD_SHIFT;
					else if (modKeys & (KMOD_LCTRL | KMOD_RCTRL)) mod = GAME_TAP_MOD_CTRL;

					if (nFingers > 1) {
						// Do nothing.
						// Multi touch in progress.
					}
					else if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
						if (!mouseDown.IsZero()) {
							#ifdef OUTPUT_MOUSE_AND_TOUCH
							GLOUTPUT(("Left Mouse move %d %d\n", x, y));
							#endif
							GameTap(game, GAME_TAP_MOVE, x, y, mod);
//							mouseMoveCount = 0;
						}
					}
					else if (!rightMouseDown.IsZero()) {
						GLASSERT(state & SDL_BUTTON(SDL_BUTTON_RIGHT));
						GameCameraPan(game, GAME_PAN_END, float(x), float(y));
					}
					else if (zooming && (state & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
						float deltaZoom = 0.01f * (float)zoomY;
						GameZoom(game, GAME_ZOOM_DISTANCE, deltaZoom);
						GameCameraRotate(game, (float)(zoomX)*0.5f);
					}
					else if (state ==0) {
						GameTap(game, GAME_MOVE_WHILE_UP, x, y, mod);
					}
				}
				break;

				case SDL_MOUSEWHEEL:
				{
					if (event.wheel.y) {
						float deltaZoom = -0.1f * float(event.wheel.y);
						GameZoom(game, GAME_ZOOM_DISTANCE, deltaZoom);
					}
				}
				break;

				case SDL_FINGERUP:
				{
					const SDL_TouchFingerEvent* tfe = &event.tfinger;
					nFingers = SDL_GetNumTouchFingers(tfe->touchId);
					if (nFingers < 2 && !multiTouchStart.IsZero()) {
						#ifdef OUTPUT_MOUSE_AND_TOUCH
						GLOUTPUT(("2 finger END.\n"));
						#endif
						multiTouchStart.Zero();
					}
				}
				break;

				case SDL_FINGERDOWN:
				{
					const SDL_TouchFingerEvent* tfe = &event.tfinger;
					nFingers = SDL_GetNumTouchFingers(tfe->touchId);
					if (nFingers > 1) {
						if (!mouseDown.IsZero()) {
							// Wrap up the existing action.
							#ifdef OUTPUT_MOUSE_AND_TOUCH
							GLOUTPUT(("Switch to gesture.\n"));
							#endif
							mouseDown.Zero();
						}
					}
				}
				break;

				case SDL_MULTIGESTURE:
				{
					const SDL_MultiGestureEvent* mge = &event.mgesture;
					nFingers = SDL_GetNumTouchFingers(mge->touchId);
					if (nFingers > 1 && multiTouchStart.IsZero()) {
						#ifdef OUTPUT_MOUSE_AND_TOUCH
						GLOUTPUT(("2 finger START.\n"));
						#endif
						multiTouchStart.Set(mge->x, mge->y);
					}
					else if (!multiTouchStart.IsZero()) {
						// The Pan interacts badly with zoom and rotated. So instead of a continuous,
						// multi-event action do a bunch of "mini pans".
						GameCameraPan(game, GAME_PAN_START,
									  multiTouchStart.x * float(screenWidth), multiTouchStart.y*float(screenHeight));
						multiTouchStart.Set(mge->x, mge->y);
						GameCameraPan(game, GAME_PAN_MOVE,
									  multiTouchStart.x * float(screenWidth), multiTouchStart.y*float(screenHeight));
						GameCameraPan(game, GAME_PAN_END,
									  multiTouchStart.x * float(screenWidth), multiTouchStart.y*float(screenHeight));
					}

					if (nFingers == 2) {
						GameZoom(game, GAME_ZOOM_DISTANCE, -mge->dDist * 10.f);
						GameCameraRotate(game, -mge->dTheta * 100.0f);
						//GLOUTPUT(("MultiGestureEvent dTheta=%.4f dDist=%.4f x=%.4f y=%.4f nFing=%d\n", mge->dTheta, mge->dDist, mge->x, mge->y, mge->numFingers));
					}
				}
				break;

				case SDL_DOLLARGESTURE:
				{
					GLOUTPUT(("DollarGestureEvent\n"));
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
		if (delta < TIME_BETWEEN_FRAMES) {
			SDL_Delay(1);
			continue;
		}
		tickTimer = SDL_GetTicks();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		const U8* keys = SDL_GetKeyboardState(0);
		U32 tickDelta = thisTick - lastTick;
		if (tickDelta > 100) tickDelta = 100;

		float keyMoveSpeed = KEY_MOVE_SPEED * float(tickDelta) / float(TIME_BETWEEN_FRAMES);
		float keyZoomSpeed = KEY_ZOOM_SPEED * float(tickDelta) / float(TIME_BETWEEN_FRAMES);
		float keyRotatepeed = KEY_ROTATE_SPEED * float(tickDelta) / float(TIME_BETWEEN_FRAMES);

		if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) {
			if (modKeys & KMOD_CTRL)
				GameZoom(game, GAME_ZOOM_DISTANCE, keyZoomSpeed);
			else
				GameCameraMove(game, 0, -keyMoveSpeed);
		}
		if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) {
			if (modKeys & KMOD_CTRL)
				GameZoom(game, GAME_ZOOM_DISTANCE, -keyZoomSpeed);
			else
				GameCameraMove(game, 0, keyMoveSpeed);
		}
		if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
			if (modKeys & KMOD_CTRL)
				GameCameraRotate(game, keyRotatepeed);
			else
				GameCameraMove(game, -keyMoveSpeed, 0);
		}
		if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
			if (modKeys & KMOD_CTRL)
				GameCameraRotate(game, -keyRotatepeed);
			else
				GameCameraMove(game, keyMoveSpeed, 0);
		}

		{
			lastTick = thisTick;
			thisTick = SDL_GetTicks();
			PROFILE_BLOCK(GameDoTick);
			GameDoTick(game, thisTick);
		}
		{
			PROFILE_BLOCK(Swap);
			SDL_GL_SwapWindow(screen);
		}
	}

	GameSave(game);
	DeleteGame(game);

	for (int i = 0; i < nModDB; ++i) {
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


void ScreenCapture()
{
	int viewPort[4];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	int width = viewPort[2] - viewPort[0];
	int height = viewPort[3] - viewPort[1];

	SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0xff, 0xff << 8, 0xff << 16, 0xff << 24);
	if (!surface)
		return;

	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	// This is a fancy swap, for the screen pixels:
	int i;
	U32* buffer = new U32[width];

	for (i = 0; i < height / 2; ++i)
	{
		memcpy(buffer,
			   ((U32*)surface->pixels + i*width),
			   width * 4);
		memcpy(((U32*)surface->pixels + i*width),
			   ((U32*)surface->pixels + (height - 1 - i)*width),
			   width * 4);
		memcpy(((U32*)surface->pixels + (height - 1 - i)*width),
			   buffer,
			   width * 4);
	}
	delete[] buffer;

	// And now, set all the alphas to opaque:
	for (i = 0; i < width*height; ++i)
		*((U32*)surface->pixels + i) |= 0xff000000;

	grinliz::Rectangle2I r;
	r.Set(0, 0, width - 1, height - 1);
	if (width == 0 || height == 0) {
		return;
	}

	int index = 0;
	grinliz::GLString path;
	for (index = 0; index < 100; ++index)
	{
		char buf[32];
		grinliz::SNPrintf(buf, 32, "cap%02d.png", index);
		GetSystemPath(GAME_SAVE_DIR, buf, &path);
		FILE* fp = fopen(path.c_str(), "rb");
		if (fp)
			fclose(fp);
		else
			break;
	}
	if (index < 100) {
		lodepng_encode32_file(path.c_str(), (const unsigned char*)surface->pixels, width, height);
	}
	SDL_FreeSurface(surface);
}
