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

class WorldMap;
class SpaceTree;

// Allows the Map to call out for the state of locations.
// The tricky bit is to remember to call the pather->Reset()
// if the isBlocked changes.
class IMapGridUse
{
public:
	// Mask of: GRID_IN_USE, GRID_BLOCKED
	virtual int MapGridUse( int x, int y ) = 0;
};


class Map : public gamui::IGamuiRenderer
{
public:

	Map( int width, int height );
	virtual ~Map();

	// The size of the map in use, which is <=SIZE
	int Height() const { return height; }
	int Width()  const { return width; }
	grinliz::Rectangle2I Bounds() const		{	return grinliz::Rectangle2I( 0, 0, width-1, height-1 ); }
	grinliz::Rectangle2F BoundsF() const	{	return grinliz::Rectangle2F( 0, 0, (float)width, (float)height ); }
	grinliz::Rectangle3F Bounds3() const	{	
		grinliz::Rectangle3F r;
		r.Set( 0, 0, 0, (float)width, MAP_HEIGHT, (float)height );
		return r;
	}
	virtual WorldMap* ToWorldMap() { return 0; }

	void DrawOverlay( int layer );
	virtual void Submit( GPUState* shader, bool emissiveOnly )	{}

	virtual void PrepVoxels( const SpaceTree* )											{}
	virtual void DrawVoxels( GPUState* state, const grinliz::Matrix4* xform )		{}
	virtual void Draw3D( const grinliz::Color3F& colorMult, GPUState::StencilMode )	{}

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

	virtual grinliz::Vector3I IntersectVoxel(	const grinliz::Vector3F& origin,
												const grinliz::Vector3F& dir,
												float length,				
												grinliz::Vector3F* intersection ) 
	{ 
		grinliz::Vector3I v = { -1, -1, -1 }; 
		return v; 
	}

	gamui::Gamui overlay0;	// overlay map, under models
	gamui::Gamui overlay1;	// overlay map, over models

	bool UsingSectors() const				{ return usingSectors; }

protected:
	bool usingSectors;
	int width;
	int height;
	CompositingShader	gamuiShader;
	Texture* texture;
};

#endif // UFOATTACK_MAP_INCLUDED
