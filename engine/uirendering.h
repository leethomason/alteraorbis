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

#ifndef UIRENDERING_INCLUDED
#define UIRENDERING_INCLUDED

#include "../gamui/gamuifreetype.h"
#include "gpustatemanager.h"
#include "texture.h"

class Screenport;

class FontSingleton : public gamui::GamuiFreetypeBridge, 
					  public ITextureCreator

{
public:
	~FontSingleton();

	static FontSingleton* Instance() {
		if (!instance)
			instance = new FontSingleton();
		return instance;
	}

	// Sets the physical pixels. Generate a new texture if needed.
	void SetPhysicalPixel(int h);

	virtual void CreateTexture(Texture* t);
	gamui::RenderAtom TextAtom(bool disabled);

private:
	enum { TEX_W = 512, TEX_H = 256 };
	FontSingleton();

	static FontSingleton* instance;
	int fontHeight;
	Texture* fontTexture;
};


class UIRenderer : public gamui::IGamuiRenderer
{
public:
	enum {
		NUM_DATA = 4,

		RENDERSTATE_UI_NORMAL = 1,		// a=1, normal UI rendering (1.0 alpha)
		RENDERSTATE_UI_NORMAL_OPAQUE,	// a=1, same, but for resources that don't blend (background images)
		RENDERSTATE_UI_GRAYSCALE_OPAQUE,// a=1, grayscale, but for resources that don't blend (background images)
		RENDERSTATE_UI_DISABLED,		// a=0.5, disabled of both above
		RENDERSTATE_UI_TEXT,			// text rendering
		RENDERSTATE_UI_TEXT_DISABLED,
		RENDERSTATE_UI_DECO,			// a=0.7,deco rendering
		RENDERSTATE_UI_DECO_DISABLED,	// a=0.2

		// Giant hack: renderstates for up to 4 faces in the UI.
		RENDERSTATE_UI_CLIP_XFORM_MAP_0,	// special rendering for faces.
		RENDERSTATE_UI_CLIP_XFORM_MAP_1,	// special rendering for faces.
		RENDERSTATE_UI_CLIP_XFORM_MAP_2,	// special rendering for faces.
		RENDERSTATE_UI_CLIP_XFORM_MAP_3,	// special rendering for faces.
		RENDERSTATE_COUNT
	};

	UIRenderer(int _layer) : layer(_layer), shader(BLEND_NORMAL), texture(0), vbo(0), ibo() {}

	virtual void BeginRender(int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex);
	virtual void EndRender();

	virtual void BeginRenderState(const void* renderState);
	virtual void BeginTexture(const void* textureHandle);
	virtual void Render(const void* renderState, const void* textureHandle, int start, int count);

	static void SetAtomCoordFromPixel(int x0, int y0, int x1, int y1, int w, int h, gamui::RenderAtom*);

	// Rendering parameters:
	grinliz::Vector4F uv[NUM_DATA];
	grinliz::Vector4F uvClip[NUM_DATA];
	grinliz::Matrix4  colorXForm[NUM_DATA];

private:
	int layer;
	CompositingShader shader;
	Texture* texture;
	GPUVertexBuffer* vbo;
	GPUIndexBuffer*  ibo;
};


class DecoEffect
{
public:
	DecoEffect() : startPauseTime( 0 ), 
		           invisibleWhenDone( false ), 
				   rotation( 0 ), 
				   item( 0 ) {}

	void Attach( gamui::UIItem* item )						{ this->item = item; }
	void Play( int startPauseTime, bool invisibleWhenDone );
	void DoTick( U32 delta );

private:
	int startPauseTime;
	bool invisibleWhenDone;
	float rotation;
	gamui::UIItem* item;
};

#endif // UIRENDERING_INCLUDED
