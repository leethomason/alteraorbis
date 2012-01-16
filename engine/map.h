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
#include "../grinliz/glbitarray.h"
#include "../grinliz/glmemorypool.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"

#include "../micropather/micropather.h"

#include "../shared/glmap.h"

#include "../gamui/gamui.h"

#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "surface.h"
#include "texture.h"
#include "gpustatemanager.h"

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;
class ParticleSystem;
class TiXmlElement;
class Map;


class Map : //public micropather::Graph,
			public ITextureCreator,
			public gamui::IGamuiRenderer
{
public:

	/*
	struct MapItem
	{
		enum {
			MI_NOT_IN_DATABASE		= 0x02,		// lights are usually generated, and are not stored in the DB
			MI_DOOR					= 0x04,
		};

		struct MatPacked {
			S8	a, b, c, d;
			S16	x, y;
		};

		U8			open;
		U8			modelRotation;
		U8			amountObscuring;		// if !0, amount to decrement 
		U16			hp;
		U16			flags;
		MatPacked	xform;			// xform in map coordinates
		grinliz::Rectangle2<U8> mapBounds8;
		float		modelX, modelZ;
		
		const MapItemDef* def;
		Model*	model;
		
		MapItem* next;			// the 'next' after a query
		MapItem* nextQuad;		// next pointer in the quadTree

		grinliz::Rectangle2I MapBounds() const 
		{	
			return grinliz::Rectangle2I( mapBounds8.min.x, mapBounds8.min.y, mapBounds8.max.x, mapBounds8.max.y );
		}
		Matrix2I XForm() const {
			Matrix2I m;
			m.a = xform.a;	m.b = xform.b; 
			m.c = xform.c;	m.d = xform.d; 
			m.x = xform.x;	m.y = xform.y;
			return m;
		}
		void SetXForm( const Matrix2I& m ) {
			GLASSERT( m.a > -128 && m.a < 128 );
			GLASSERT( m.b > -128 && m.b < 128 );
			GLASSERT( m.c > -128 && m.c < 128 );
			GLASSERT( m.d > -128 && m.d < 128 );
			GLASSERT( m.x > -30000 && m.x < 30000 );
			GLASSERT( m.y > -30000 && m.y < 30000 );
			xform.a = (S8)m.a;
			xform.b = (S8)m.b;
			xform.c = (S8)m.c;
			xform.d = (S8)m.d;
			xform.x = (S16)m.x;
			xform.y = (S16)m.y;
		}

		grinliz::Vector3F ModelPos() const { 
			grinliz::Vector3F v = { modelX, 0.0f, modelZ };
			return v;
		}
		float ModelRot() const { return (float)(modelRotation*90); }

		// returns true if destroyed
		bool DoDamage( int _hp )		
		{	
			if ( _hp >= hp ) {
				hp = 0;
				return true;
			}
			hp -= _hp;
			return false;						
		}
		bool Destroyed() const { return hp == 0; }
	};
	*/

	Map( SpaceTree* tree );
	virtual ~Map();

	// The size of the map in use, which is <=SIZE
	int Height() const { return height; }
	int Width()  const { return width; }
	grinliz::Rectangle2I Bounds() const		{	return grinliz::Rectangle2I( 0, 0, width-1, height-1 ); }

	// Light Map
	// 0: Light map that was set in "SetLightMap", or white if none set
	// Not currently used: 1: Light map 0 + lights
	// Not currently used: 2: Light map 0 + lights + FogOfWar
	const Surface* GetLightMap()	{ return 0; }

	void EmitParticles( U32 deltaTime )			{}
	void DrawOverlay()							{}

	/*
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );
	*/
	// ITextureCreator
	virtual void CreateTexture( Texture* t );

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

//	Texture* BackgroundTexture()	{ return backgroundTexture; }
//	Texture* LightMapTexture()		{ return lightMapTex; }
	
	void Save( FILE* fp, int depth );
	void Load( const TiXmlElement* mapNode );


protected:
	/*
	class QuadTree
	{
	public:
		enum {
			QUAD_DEPTH = 5,
			NUM_QUAD_NODES = 1+4+16+64+256,
		};

		QuadTree();
		void Clear();

		void Add( MapItem* );

		MapItem* FindItems( const grinliz::Rectangle2I& bounds, int required, int excluded );
		MapItem* FindItems( int x, int y, int required, int excluded ) 
		{ 
			grinliz::Rectangle2I b( x, y, x, y ); 
			return FindItems( b, required, excluded ); 
		}
		MapItem* FindItem( const Model* model );

		void UnlinkItem( MapItem* item );

		void MarkVisible( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& fogOfWar );

	private:
		int WorldToNode( int x, int depth )					
		{ 
			GLRELASSERT( depth >=0 && depth < QUAD_DEPTH );
			GLRELASSERT( x>=0 && x < Map::SIZE );
			return x >> (LOG2_SIZE-depth); 
		}
		int NodeToWorld( int x0, int depth )
		{
			GLRELASSERT( x0>=0 && x0 < (1<<depth) );
			return x0 << (LOG2_SIZE-depth);			
		}
		int NodeOffset( int x0, int y0, int depth )	
		{	
			GLRELASSERT( x0>=0 && x0 < (1<<depth) );
			GLRELASSERT( y0>=0 && y0 < (1<<depth) );
			return y0 * (1<<depth) + x0; 
		}

		int CalcNode( const grinliz::Rectangle2<U8>& bounds, int* depth );

		int			depthUse[QUAD_DEPTH];
		int			depthBase[QUAD_DEPTH+1];
		MapItem*	tree[NUM_QUAD_NODES];
		const Model* filterModel;
	};
	*/

	SpaceTree*	tree;
	//QuadTree	quadTree;
	int width;
	int height;
	CompositingShader							gamuiShader;
};

#endif // UFOATTACK_MAP_INCLUDED
