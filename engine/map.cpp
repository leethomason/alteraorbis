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

#include "map.h"
#include "model.h"
#include "loosequadtree.h"
#include "renderqueue.h"
#include "surface.h"
#include "text.h"
#include "vertex.h"
#include "uirendering.h"
#include "engine.h"
#include "settings.h"

#include "../engine/particle.h"

#include "../tinyxml2/tinyxml2.h"

#include "../grinliz/glstringutil.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glperformance.h"

using namespace grinliz;
using namespace tinyxml2;

const float DIAGONAL_COST = 1.414f;



Map::Map( int w, int h )
{
	usingSectors = false;
	width = w;
	height = h;
	texture = 0;
	GLASSERT( w <= EL_MAX_MAP_SIZE );
	GLASSERT( h <= EL_MAX_MAP_SIZE );
	GLOUTPUT(( "Map created. %dK\n", sizeof( *this )/1024 ));

	gamui::RenderAtom nullAtom;
	overlay.Init( this, nullAtom, nullAtom, 0 );
}


Map::~Map()
{
}


void Map::DrawOverlay()
{
	overlay.Render();
}


/*
void Map::Save( XMLPrinter* printer )
{
	printer->OpenElement( "Map" );
	printer->PushAttribute( "sizeX", width );
	printer->PushAttribute( "sizeY", height );
	printer->CloseElement();
}


void Map::Load( const XMLElement* mapElement )
{
	GLASSERT( mapElement );
}
*/

void Map::BeginRender()
{
	gamuiShader.PushMatrix( GPUState::MODELVIEW_MATRIX );

	Matrix4 rot;
	rot.SetXRotation( 90 );
	gamuiShader.MultMatrix( GPUState::MODELVIEW_MATRIX, rot );
}


void Map::EndRender()
{
	gamuiShader.PopMatrix( GPUState::MODELVIEW_MATRIX );
}


void Map::BeginRenderState( const void* renderState )
{
	const float ALPHA = 0.5f;
	const float ALPHA_1 = 0.30f;
	switch( (int)renderState ) {
		case UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE:
		case RENDERSTATE_MAP_OPAQUE:
			gamuiShader.SetColor( 1, 1, 1, 1 );
			gamuiShader.SetBlendMode( GPUState::BLEND_NONE );
			break;

		case UIRenderer::RENDERSTATE_UI_NORMAL:
		case RENDERSTATE_MAP_NORMAL:
			gamuiShader.SetColor( 1.0f, 1.0f, 1.0f, 0.8f );
			gamuiShader.SetBlendMode( GPUState::BLEND_NORMAL );
			break;

		case RENDERSTATE_MAP_TRANSLUCENT:
			gamuiShader.SetColor( 1, 1, 1, ALPHA );
			gamuiShader.SetBlendMode( GPUState::BLEND_NORMAL );
			break;
		case RENDERSTATE_MAP_MORE_TRANSLUCENT:
			gamuiShader.SetColor( 1, 1, 1, ALPHA_1 );
			gamuiShader.SetBlendMode( GPUState::BLEND_NORMAL );
			break;
		default:
			GLASSERT( 0 );
	}
}


void Map::BeginTexture( const void* textureHandle )
{
	texture = (Texture*)textureHandle;
}


void Map::Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex )
{
	GPUStream stream( GPUStream::kGamuiType );
	gamuiShader.Draw( stream, texture, vertex, nIndex, index );
}
