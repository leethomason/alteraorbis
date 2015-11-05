/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.

 3. This notice may not be removed or altered from any source
 distribution.
 */

#define BRIDGE 1

#include "sdl.h"
#include "../engine/platformgl.h"
#include "../engine/texture.h"
#include "../engine/screenport.h"
#include "../engine/gpustatemanager.h"
#include "../engine/uirendering.h"

#include "gamui.h"
#include "gamuifreetype.h"

#include <stdio.h>
#include <math.h>

using namespace gamui;

const int VIRTUAL_X = 600;
const int VIRTUAL_Y = 400;

int screenX = VIRTUAL_X;
int screenY = VIRTUAL_Y;

class TextMetrics : public IGamuiText
{
public:
	enum { height = 16 };
	virtual void GamuiGlyph(int c, int c1, GlyphMetrics* metric)
	{
		if (c <= 32 || c >= 32 + 96) {
			c = 32;
		}
		c -= 32;
		int y = c / 16;
		int x = c - y * 16;

		float tx0 = (float)x / 16.0f;
		float ty0 = (float)y / 8.0f;

		metric->advance = 10;
		metric->x = -3.f;
		metric->w = 16.f;
		metric->y = -floorf(float(height) * 0.75f);	// baseline position. assume baseline 75% of glyph
		metric->h = 16.f;

		metric->tx0 = tx0;
		metric->tx1 = tx0 + (1.f / 16.f);

		metric->ty0 = ty0;
		metric->ty1 = ty0 + (1.f / 8.f);
	}

	virtual void GamuiFont(gamui::IGamuiText::FontMetrics* metric)
	{
		metric->ascent = int(height * 0.75f);
		metric->descent = int(height * 0.25f);
		metric->linespace = int(height);
	}
};

static const int TEXT_TEXTURE_SIZE = 512;

void GenerateTextTexture(int textHeightInPixels, GamuiFreetypeBridge* bridge, Texture* textTexture)
{
	uint8_t* pixels = new uint8_t[TEXT_TEXTURE_SIZE*TEXT_TEXTURE_SIZE];
	bridge->Generate(textHeightInPixels, pixels, TEXT_TEXTURE_SIZE, TEXT_TEXTURE_SIZE, true);
	textTexture->UploadAlphaToRGBA16(pixels, TEXT_TEXTURE_SIZE * TEXT_TEXTURE_SIZE);
	delete[] pixels;
}



int main(int argc, char **argv)
{
	printf("GamuiTest started.\n");

	// SDL initialization steps.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL initialization failed: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window *screen = SDL_CreateWindow("Gamui",
										  50, 50,
										  screenX, screenY,
										  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GL_CreateContext(screen);
	glewInit();
	Screenport screenport(screenX, screenY);
	screenport.SetUI(GPUDevice::Instance());
	printf("Window created.\n");

	// Load text texture
	Texture* textTexture = new Texture();
	{
#if BRIDGE == 0
		SDL_Surface* textSurface = SDL_LoadBMP("./gamui/stdfont2.bmp");
		textTexture->Set("text", textSurface->w, textSurface->h, TextureType::TEX_RGBA16, Texture::PARAM_NEAREST);
		textTexture->UploadAlphaToRGBA16((const uint8_t*)textSurface->pixels, textSurface->pitch * textSurface->h);
		SDL_FreeSurface(textSurface);
#else
		textTexture->Set("text", TEXT_TEXTURE_SIZE, TEXT_TEXTURE_SIZE, TextureType::TEX_RGBA16, Texture::PARAM_NEAREST);
#endif
	}
	CHECK_GL_ERROR;

	Texture* imageTexture = new Texture();
	{
		// Load a bitmap, and flip the R-B byte order.
		SDL_Surface* surface = SDL_LoadBMP("./gamui/buttons.bmp");
		SDL_Surface* imageSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, 24, 0xff, 0xff00, 0xff0000, 0);
		SDL_BlitSurface(surface, 0, imageSurface, 0);

		imageTexture->Set("image", imageSurface->w, imageSurface->h, TextureType::TEX_RGB24, 0);
		imageTexture->Upload((const uint8_t*)imageSurface->pixels, imageSurface->pitch * imageSurface->h);

		SDL_FreeSurface(surface);
		SDL_FreeSurface(imageSurface);
	}

	RenderAtom nullAtom;

	// 256, 128
	RenderAtom textAtom(UIRenderer::RENDERSTATE_UI_TEXT, textTexture, 0, 0, 0, 0);
	RenderAtom textAtomD = textAtom;
	textAtomD.renderState = (const void*)UIRenderer::RENDERSTATE_UI_TEXT_DISABLED;

	RenderAtom whiteAtom(UIRenderer::RENDERSTATE_UI_NORMAL, imageTexture, 0.1f, 0.4f, 0.2f, 0.45f);

	// 100x100
	RenderAtom imageAtom(UIRenderer::RENDERSTATE_UI_NORMAL, imageTexture, 0.5f, 0.5f, 228.f / 256.f, 28.f / 256.f);

	// 50x50
	//RenderAtom decoAtom( RENDERSTATE_NORMAL, imageTextureID, 0, 0.25f, 0.25f, 0.f );
	RenderAtom decoAtom = nullAtom;
	RenderAtom decoAtomD = decoAtom;
	decoAtomD.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DISABLED;

	TextMetrics textMetrics;
	UIRenderer renderer(0);

	CHECK_GL_ERROR;
	Gamui gamui;
	gamui.Init(&renderer);
#if BRIDGE == 0
	gamui.SetText(textAtom, textAtomD, &textMetrics);
#endif
	gamui.SetScale(screenX, screenY, VIRTUAL_Y);

#if BRIDGE == 1
	GamuiFreetypeBridge* bridge = new GamuiFreetypeBridge();
	//bridge->Init("./gamui/OpenSans-Regular.ttf");
	bridge->Init("./gamui/DidactGothic.ttf");
	int textHeightInPixels = (int)gamui.TransformVirtualToPhysical(16);
	GenerateTextTexture(textHeightInPixels, bridge, textTexture);

	textAtom.textureHandle  = textTexture;
	textAtomD.textureHandle = textTexture;
	gamui.SetText(textAtom, textAtomD, bridge);
#endif

	TextLabel textLabel[2];
	textLabel[0].Init(&gamui);
	textLabel[1].Init(&gamui);

	textLabel[0].SetText("Hello Gamui. This is text\n"
						 "with line breaks, that is\n"
						 "positioned at the origin.");
	textLabel[1].SetText("Very long text to test the string allocator.\nAnd some classic kerns: VA AV.");
	textLabel[1].SetPos(10, 200);

	Image image0;
	image0.Init(&gamui, imageAtom, true);
	image0.SetPos(50, 50);
	image0.SetSize(100, 100);

	Image image1;
	image1.Init(&gamui, imageAtom, true);
	image1.SetPos(50, 200);
	image1.SetSize(125, 125);

	Image image2;
	image2.Init(&gamui, imageAtom, true);
	image2.SetPos(200, 50);
	image2.SetSize(50, 50);

	Image image2b;
	image2b.Init(&gamui, imageAtom, true);
	image2b.SetPos(270, 50);
	image2b.SetSize(50, 50);

	Image image2c;
	image2c.Init(&gamui, imageAtom, true);
	image2c.SetPos(200, 120);
	image2c.SetSize(50, 50);

	Image image2d;
	image2d.Init(&gamui, imageAtom, true);
	image2d.SetPos(270, 120);
	image2d.SetSize(50, 50);

	Image image3;
	image3.Init(&gamui, imageAtom, true);
	image3.SetPos(200, 200);
	image3.SetSize(125, 125);
	image3.SetSlice(true);

	Canvas canvas;
	canvas.Init(&gamui, whiteAtom);
	canvas.SetPos(500, 0);
	canvas.DrawRectangleOutline(10, 10, 80, 80, 4, 10);

	// 50x50
	RenderAtom up(UIRenderer::RENDERSTATE_UI_NORMAL, imageTexture, 0, 1, (52.f / 256.f), (204.f / 256.f));
	RenderAtom upD = up;
	upD.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DISABLED;

	// 50x50
	RenderAtom down(UIRenderer::RENDERSTATE_UI_NORMAL, imageTexture, 0, 0.75f, (52.f / 256.f), (140.f / 256.f));
	RenderAtom downD(down, UIRenderer::RENDERSTATE_UI_DISABLED);

	PushButton button0(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	button0.SetPos(350, 50);
	button0.SetSize(150, 75);
	button0.SetText("Button");

	PushButton button1(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	button1.SetPos(350, 150);
	button1.SetSize(150, 75);
	button1.SetText("Button");
	button1.SetEnabled(false);

	ToggleButton toggle(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	toggle.SetPos(350, 250);
	toggle.SetSize(150, 50);
	toggle.SetText("Toggle\nLine 2");

	ToggleButton toggle0(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	toggle0.SetPos(350, 325);
	toggle0.SetSize(75, 50);
	toggle0.SetText("group");

	ToggleButton toggle1(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	toggle1.SetPos(430, 325);
	toggle1.SetSize(75, 50);
	toggle1.SetText("group");

	ToggleButton toggle2(&gamui, up, upD, down, downD, decoAtom, decoAtomD);
	toggle2.SetPos(510, 325);
	toggle2.SetSize(75, 50);
	toggle2.SetText("group");

	toggle0.AddToToggleGroup(&toggle1);
	toggle0.AddToToggleGroup(&toggle2);

	// 15x30
	RenderAtom tick0(UIRenderer::RENDERSTATE_UI_NORMAL, imageTexture, 0, 0, 0, 0);
	RenderAtom tick1 = tick0, tick2 = tick0;
	tick0.SetCoord(190.f / 256.f, 225.f / 256.f, 205.f / 256.f, 1);
	tick2.SetCoord(190.f / 256.f, 180.f / 256.f, 205.f / 256.f, 210.f / 256.f);
	tick1.SetCoord(230.f / 256.f, 225.f / 256.f, 245.f / 256.f, 1);

	DigitalBar bar;
	bar.Init(&gamui, tick0, tick1);
	bar.SetPos(20, 350);
	bar.SetSize(100, 20);

	TiledImage<2, 2> tiled(&gamui);
	tiled.SetPos(520, 20);
	tiled.SetSize(50, 50);
	tiled.SetTile(0, 0, tick1);
	tiled.SetTile(1, 0, nullAtom);
	tiled.SetTile(0, 1, nullAtom);
	tiled.SetTile(1, 1, tick0);

	bool done = false;
	float range = 0.5f;
	while (!done) {
		SDL_Event event;
		if (SDL_PollEvent(&event)) {
			switch (event.type) {

				case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					screenX = event.window.data1;
					screenY = event.window.data2;
					screenport.Resize(screenX, screenY, GPUDevice::Instance());

					gamui.SetScale(screenX, screenY, VIRTUAL_Y);
#if BRIDGE == 0
					gamui.SetText(textAtom, textAtomD, &textMetrics);
#else	
					int textHeightInPixels = (int)gamui.TransformVirtualToPhysical(16);
					GenerateTextTexture(textHeightInPixels, bridge, textTexture);
					textAtom.textureHandle  = textTexture;
					textAtomD.textureHandle = textTexture;
					gamui.SetText(textAtom, textAtomD, bridge);
#endif
				}
				break;

				case SDL_KEYDOWN:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						done = true;
						break;
					}
				}
				break;

				case SDL_MOUSEBUTTONDOWN:
				gamui.TapDown((float)event.button.x, (float)event.button.y);
				break;

				case SDL_MOUSEBUTTONUP:
				{
					const UIItem* item = gamui.TapUp((float)event.button.x, (float)event.button.y);
					if (item) {
						range += 0.1f;
						if (range > 1.0f)
							range = 0.0f;
						//							bar.SetRange( 0, range );
					}
				}
				break;

				case SDL_QUIT:
				done = true;
				break;
			}
		}
		GPUDevice::Instance()->Clear(0, 0, 0, 0);

		float rotation = (float)((double)SDL_GetTicks() * 0.05);
		image2b.SetRotationX(rotation);
		image2c.SetRotationY(rotation);
		image2d.SetRotationZ(rotation);

		screenport.SetUI(GPUDevice::Instance());
		gamui.Render();

		SDL_GL_SwapWindow(screen);
	}
	delete textTexture;
#if BRIDGE == 1
	delete bridge; bridge = 0;
#endif
	gamui.SetText(textAtom, textAtomD, 0);
	delete GPUDevice::Instance();
	SDL_Quit();
	return 0;
}
