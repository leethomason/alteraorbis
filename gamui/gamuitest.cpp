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

#include "gamui.h"
#include "gamuifreetype.h"

#include <stdio.h>
#include <math.h>

using namespace gamui;

enum {
	RENDERSTATE_TEXT = 1,
	RENDERSTATE_TEXT_DISABLED,
	RENDERSTATE_NORMAL,
	RENDERSTATE_DISABLED
};

const int VIRTUAL_X = 600;
const int VIRTUAL_Y = 400;

int screenX = VIRTUAL_X;
int screenY = VIRTUAL_Y;

class Renderer : public IGamuiRenderer
{
	const uint16_t* m_index;
	const Gamui::Vertex* m_vertex;

public:
	virtual void BeginRender( int nIndex, const uint16_t* index, int nVertex, const Gamui::Vertex* vertex )
	{
		CHECK_GL_ERROR;

		m_index = index;
		m_vertex = vertex;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( 0, screenX, screenY, 0, 1, -1 );

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();				// model

		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
		glEnable( GL_BLEND );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable( GL_TEXTURE_2D );

		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		CHECK_GL_ERROR;
	}

	virtual void EndRender() 
	{
		CHECK_GL_ERROR;

	}

	virtual void BeginRenderState( const void* _renderState )
	{
		CHECK_GL_ERROR;
		intptr_t renderState = (intptr_t)(_renderState);

#if 0
		if ( renderState == RENDERSTATE_TEXT )
			glDisable( GL_BLEND );
		else
			glEnable( GL_BLEND );
#endif

		switch( renderState ) {
			case RENDERSTATE_TEXT:
			case RENDERSTATE_NORMAL:
				glColor4f( 1.f, 1.f, 1.f, 1.f );
				break;

			case RENDERSTATE_DISABLED:
			case RENDERSTATE_TEXT_DISABLED:
				glColor4f( 1.f, 1.f, 1.f, 0.5f );
				break;

			default:
				GAMUIASSERT( 0 );
				break;
		}
		CHECK_GL_ERROR;
	}

	virtual void BeginTexture( const void* _textureHandle )
	{
		CHECK_GL_ERROR;
		intptr_t textureHandle = (intptr_t)_textureHandle;
		glBindTexture( GL_TEXTURE_2D, (GLuint)textureHandle );
		CHECK_GL_ERROR;
	}


	virtual void Render( const void* renderState, const void* textureHandle, int start, int count )
	{
		CHECK_GL_ERROR;
		glVertexPointer( 2, GL_FLOAT, sizeof(Gamui::Vertex), &m_vertex->x );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(Gamui::Vertex), &m_vertex->tx );
		glDrawElements( GL_TRIANGLES, count, GL_UNSIGNED_SHORT, m_index + start );
		CHECK_GL_ERROR;
	}
};


class TextMetrics : public IGamuiText
{
public:
	enum { height = 16};
	virtual void GamuiGlyph( int c, int c1, GlyphMetrics* metric )
	{
		if ( c <= 32 || c >= 32+96 ) {
			c = 32;
		}
		c -= 32;
		int y = c / 16;
		int x = c - y*16;

		float tx0 = (float)x / 16.0f;
		float ty0 = (float)y / 8.0f;

		metric->advance = 10;
		metric->x = -3.f;
		metric->w = 16.f;
		metric->y = -floorf(float(height) * 0.75f);	// baseline position. assume baseline 75% of glyph
		metric->h = 16.f;

		metric->tx0 = tx0;
		metric->tx1 = tx0 + (1.f/16.f);

		metric->ty0 = ty0;
		metric->ty1 = ty0 + (1.f/8.f);
	}

	virtual void GamuiFont(gamui::IGamuiText::FontMetrics* metric)
	{
		metric->ascent = int(height * 0.75f);
		metric->descent = int(height * 0.25f);
		metric->linespace = int(height);
	}
};


int main( int argc, char **argv )
{    
	printf("GamuiTest started.\n");

	// SDL initialization steps.
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO ) < 0 )
	{
	    printf( "SDL initialization failed: %s\n", SDL_GetError( ) );
		exit( 1 );
	}

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8);

	SDL_Window *screen = SDL_CreateWindow(	"Gamui",
											50, 50,
											screenX, screenY,
											SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
	SDL_GL_CreateContext( screen );
	glewInit();
	glViewport(0, 0, screenX, screenY);
	printf("Window created.\n");

	// Load text texture
	SDL_Surface* textSurface = SDL_LoadBMP( "./gamui/stdfont2.bmp" );
	Texture* textTexture = new Texture();
	textTexture->Set("text", textSurface->w, textSurface->h, TextureType::TEX_RGBA16, Texture::PARAM_NEAREST);
	textTexture->UploadAlphaToRGBA16((const uint8_t*) textSurface->pixels, textSurface->pitch * textSurface->h);

	CHECK_GL_ERROR;
	GLuint textTextureID;
	glGenTextures( 1, &textTextureID );
	glBindTexture( GL_TEXTURE_2D, textTextureID );

	//glTexParameteri(	GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(	GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0,	GL_ALPHA, textSurface->w, textSurface->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textSurface->pixels );
	CHECK_GL_ERROR;

	// Load a bitmap
	SDL_Surface* imageSurface = SDL_LoadBMP( "./gamui/buttons.bmp" );
	for (int j = 0; j < imageSurface->h; ++j) {
		for (int i = 0; i < imageSurface->w; ++i) {
			uint8_t* p0 = (uint8_t*)imageSurface->pixels + j * imageSurface->pitch + i * 3;
			uint8_t* p1 = p0 + 2;
			uint8_t t = *p0;
			*p0 = *p1;
			*p1 = t;
		}
	}

	GLuint imageTextureID;
	glGenTextures( 1, &imageTextureID );
	glBindTexture( GL_TEXTURE_2D, imageTextureID );

	glTexParameteri(	GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	glTexParameteri(	GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri(	GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0,	GL_RGBA, imageSurface->w, imageSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, imageSurface->pixels );
	CHECK_GL_ERROR;
	SDL_FreeSurface( imageSurface );

	RenderAtom nullAtom;

	// 256, 128
	RenderAtom textAtom( (const void*)RENDERSTATE_TEXT, (const void*)textTextureID, 0, 0, 0, 0 );
	RenderAtom textAtomD = textAtom;
	textAtomD.renderState = (const void*) RENDERSTATE_TEXT_DISABLED;

	RenderAtom whiteAtom((const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0.1f, 0.4f, 0.2f, 0.45f);

	// 100x100
	RenderAtom imageAtom( (const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0.5f, 0.5f, 228.f/256.f, 28.f/256.f );

	// 50x50
	//RenderAtom decoAtom( (const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0, 0.25f, 0.25f, 0.f );
	RenderAtom decoAtom = nullAtom; 
	RenderAtom decoAtomD = decoAtom;
	decoAtomD.renderState = (const void*) RENDERSTATE_DISABLED;

	TextMetrics textMetrics;
	Renderer renderer;

	CHECK_GL_ERROR;
	Gamui gamui;
	gamui.Init(&renderer);
#if BRIDGE == 0
	gamui.SetText(16, textAtom, textAtomD, &textMetrics);
#endif
	gamui.SetScale(screenX, screenY, VIRTUAL_Y);

	GamuiFreetypeBridge* bridge = new GamuiFreetypeBridge();
	//bridge->Init("./gamui/OpenSans-Regular.ttf");
	bridge->Init("./gamui/DidactGothic.ttf");
	SDL_Surface* fontSurface = SDL_CreateRGBSurface(0, 256, 256, 8, 0, 0, 0, 0);
	GAMUIASSERT(fontSurface->w == fontSurface->pitch);
	SDL_SetSurfacePalette(fontSurface, textSurface->format->palette);

	int textHeightInPixels = (int)gamui.TransformVirtualToPhysical(16);
	bridge->Generate(textHeightInPixels, (uint8_t*)fontSurface->pixels, fontSurface->w, fontSurface->h, true);
	SDL_SaveBMP(fontSurface, "testfontsurface.bmp");
	SDL_SaveBMP(textSurface, "testtextsurface.bmp");

#if BRIDGE == 1
	CHECK_GL_ERROR;
	GLuint textTextureID2;
	glGenTextures(1, &textTextureID2);
	glBindTexture(GL_TEXTURE_2D, textTextureID2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fontSurface->w, fontSurface->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, fontSurface->pixels);
	CHECK_GL_ERROR;

	textAtom.textureHandle = (const void*)textTextureID2;
	textAtomD.textureHandle = textAtom.textureHandle;
	gamui.SetText(textAtom, textAtomD, bridge);
#endif

	TextLabel textLabel[2];
	textLabel[0].Init( &gamui );
	textLabel[1].Init( &gamui );

	textLabel[0].SetText( "Hello Gamui. This is text\n"
						  "with line breaks, that is\n"
						  "positioned at the origin.");
	textLabel[1].SetText( "Very long text to test the string allocator.\nAnd some classic kerns: VA AV." );
	textLabel[1].SetPos( 10, 200 );

	Image image0;
	image0.Init(&gamui, imageAtom, true);
	image0.SetPos( 50, 50 );
	image0.SetSize( 100, 100 );

	Image image1;
	image1.Init(&gamui, imageAtom, true);
	image1.SetPos( 50, 200 );
	image1.SetSize( 125, 125 );

	Image image2;
	image2.Init(&gamui, imageAtom, true);
	image2.SetPos( 200, 50 );
	image2.SetSize( 50, 50 );

	Image image2b;
	image2b.Init(&gamui, imageAtom, true);
	image2b.SetPos( 270, 50 );
	image2b.SetSize( 50, 50 );

	Image image2c;
	image2c.Init(&gamui, imageAtom, true);
	image2c.SetPos( 200, 120 );
	image2c.SetSize( 50, 50 );

	Image image2d;
	image2d.Init(&gamui, imageAtom, true);
	image2d.SetPos( 270, 120 );
	image2d.SetSize( 50, 50 );

	Image image3;
	image3.Init(&gamui, imageAtom, true);
	image3.SetPos( 200, 200 );
	image3.SetSize( 125, 125 );
	image3.SetSlice( true );

	Canvas canvas;
	canvas.Init(&gamui, whiteAtom);
	canvas.SetPos(500, 0);
	canvas.DrawRectangleOutline(10, 10, 80, 80, 4, 10);

	// 50x50
	RenderAtom up( (const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0, 1, (52.f/256.f), (204.f/256.f) );
	RenderAtom upD = up;
	upD.renderState = (const void*) RENDERSTATE_DISABLED;

	// 50x50
	RenderAtom down( (const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0, 0.75f, (52.f/256.f), (140.f/256.f) );
	RenderAtom downD( down, (const void*)RENDERSTATE_DISABLED );

	PushButton button0( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	button0.SetPos( 350, 50 );
	button0.SetSize( 150, 75 );
	button0.SetText( "Button" );

	PushButton button1( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	button1.SetPos( 350, 150 );
	button1.SetSize( 150, 75 );
	button1.SetText( "Button" );
	button1.SetEnabled( false );

	ToggleButton toggle( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	toggle.SetPos( 350, 250 );
	toggle.SetSize( 150, 50 );
	toggle.SetText( "Toggle\nLine 2" );

	ToggleButton toggle0( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	toggle0.SetPos( 350, 325 );
	toggle0.SetSize( 75, 50 );
	toggle0.SetText( "group" );

	ToggleButton toggle1( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	toggle1.SetPos( 430, 325 );
	toggle1.SetSize( 75, 50 );
	toggle1.SetText( "group" );

	ToggleButton toggle2( &gamui, up, upD, down, downD, decoAtom, decoAtomD );
	toggle2.SetPos( 510, 325 );
	toggle2.SetSize( 75, 50 );
	toggle2.SetText( "group" );

	toggle0.AddToToggleGroup( &toggle1 );
	toggle0.AddToToggleGroup( &toggle2 );

	// 15x30
	RenderAtom tick0( (const void*)RENDERSTATE_NORMAL, (const void*)imageTextureID, 0, 0, 0, 0 );
	RenderAtom tick1=tick0, tick2=tick0;
	tick0.SetCoord( 190.f/256.f, 225.f/256.f, 205.f/256.f, 1 );
	tick2.SetCoord( 190.f/256.f, 180.f/256.f, 205.f/256.f, 210.f/256.f );
	tick1.SetCoord( 230.f/256.f, 225.f/256.f, 245.f/256.f, 1 );

	DigitalBar bar;
	bar.Init(&gamui, tick0, tick1);
	bar.SetPos( 20, 350 );
	bar.SetSize( 100, 20 );

	TiledImage<2, 2> tiled( &gamui );
	tiled.SetPos( 520, 20 );
	tiled.SetSize( 50, 50 );
	tiled.SetTile( 0, 0, tick1 );
	tiled.SetTile( 1, 0, nullAtom );
	tiled.SetTile( 0, 1, nullAtom );
	tiled.SetTile( 1, 1, tick0 );
	
	bool done = false;
	float range = 0.5f;
	while ( !done ) {
		SDL_Event event;
		if ( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
	
				case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					screenX = event.window.data1;
					screenY = event.window.data2;
					glViewport( 0, 0, screenX, screenY );

					gamui.SetScale(screenX, screenY, VIRTUAL_Y);
#if BRIDGE == 0
					gamui.SetText(16, textAtom, textAtomD, &textMetrics);
#else	
					int textHeightInPixels = (int)gamui.TransformVirtualToPhysical(16);
					bridge->Generate(textHeightInPixels, (uint8_t*)fontSurface->pixels, fontSurface->w, fontSurface->h, true);
					SDL_SaveBMP(fontSurface, "testfontsurface.bmp");
					CHECK_GL_ERROR;
					glBindTexture(GL_TEXTURE_2D, textTextureID2);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fontSurface->w, fontSurface->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, fontSurface->pixels);
					CHECK_GL_ERROR;
					gamui.SetText(textAtom, textAtomD, bridge);
#endif
				}
				break;

				case SDL_KEYDOWN:
				{
					switch ( event.key.keysym.sym )
					{
						case SDLK_ESCAPE:
							done = true;
							break;
					}
				}
				break;

				case SDL_MOUSEBUTTONDOWN:
					gamui.TapDown( (float)event.button.x, (float)event.button.y );
					break;

				case SDL_MOUSEBUTTONUP:
					{
						const UIItem* item = gamui.TapUp( (float)event.button.x, (float)event.button.y );
						if ( item ) {
							range += 0.1f;
							if ( range > 1.0f )
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

		glClearColor( 0, 0, 0, 1 );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float rotation = (float)((double)SDL_GetTicks() * 0.05 );
		image2b.SetRotationX( rotation );
		image2c.SetRotationY( rotation );
		image2d.SetRotationZ( rotation );
		gamui.Render();

		SDL_GL_SwapWindow( screen );
	}
	delete textTexture;
	SDL_FreeSurface(textSurface);
	SDL_FreeSurface(fontSurface);

	gamui.SetText(textAtom, textAtomD, 0);
	delete bridge; bridge = 0;
	SDL_Quit();
	return 0;
}
