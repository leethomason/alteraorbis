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

#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"

#include "../gamui/gamui.h"
#include "../tinyxml2/tinyxml2.h"

#include "vertex.h"
#include "enginelimits.h"
#include "gpustatemanager.h"


class Map : public gamui::IGamuiRenderer
{
public:

	Map( int width, int height );
	virtual ~Map();

	// The size of the map in use, which is <=SIZE
	int Height() const { return height; }
	int Width()  const { return width; }
	grinliz::Rectangle2I Bounds() const		{	return grinliz::Rectangle2I( 0, 0, width-1, height-1 ); }

	void DrawOverlay();
	virtual void Submit( GPUShader* shader, bool emissiveOnly )	{}
	virtual void Draw3D( const grinliz::Color3F& colorMult, GPUShader::StencilMode )	{}

	// IGamuiRenderer
	enum {
		RENDERSTATE_MAP_NORMAL = 100,
		RENDERSTATE_MAP_OPAQUE,
		RENDERSTATE_MAP_TRANSLUCENT,
		RENDERSTATE_MAP_MORE_TRANSLUCENT
	};
	virtual void BeginRender();
	virtual void EndRender();
	virtual void BeginRenderState( const void* renderState );
	virtual void BeginTexture( const void* textureHandle );
	virtual void Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex );

	void Save( tinyxml2::XMLPrinter*  );
	void Load( const tinyxml2::XMLElement* mapNode );

	gamui::Gamui overlay;

protected:
	int width;
	int height;
	CompositingShader	gamuiShader;
};

#endif // UFOATTACK_MAP_INCLUDED
