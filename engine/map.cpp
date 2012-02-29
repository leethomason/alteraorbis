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

#include "../engine/particleeffect.h"
#include "../engine/particle.h"

#include "../tinyxml2/tinyxml2.h"

#include "../grinliz/glstringutil.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glperformance.h"

using namespace grinliz;
using namespace micropather;


const float DIAGONAL_COST = 1.414f;


/*
const ModelResource* MapItemDef::GetModelResource() const
{
	if ( !resource )
		return 0;

	ModelResourceManager* resman = ModelResourceManager::Instance();
	return resman->GetModelResource( resource );
}


const ModelResource* MapItemDef::GetOpenResource() const 
{
	if ( !resourceOpen )
		return 0;

	ModelResourceManager* resman = ModelResourceManager::Instance();
	return resman->GetModelResource( resourceOpen );
}


const ModelResource* MapItemDef::GetDestroyedResource() const
{
	if ( !resourceDestroyed )
		return 0;

	ModelResourceManager* resman = ModelResourceManager::Instance();
	return resman->GetModelResource( resourceDestroyed );
}
*/


Map::Map( SpaceTree* tree )
{
//	microPather = new MicroPather(	this,			// graph interface
//									SIZE*SIZE,		// max possible states (+1)
//									6 );			// max adjacent states

	this->tree = tree;
	width = height = 64;
	GLOUTPUT(( "Map created. %dK\n", sizeof( *this )/1024 ));
}


Map::~Map()
{
}


void Map::Save( FILE* fp, int depth )
{
	XMLUtil::OpenElement( fp, depth, "Map" );
	XMLUtil::Attribute( fp, "sizeX", width );
	XMLUtil::Attribute( fp, "sizeY", height );
	XMLUtil::SealElement( fp );
	XMLUtil::CloseElement( fp, depth, "Map" );
}


void Map::Load( const TiXmlElement* mapElement )
{
	GLASSERT( mapElement );
}



/*
float Map::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	Vector2<S16> start, end;
	StateToVec( stateStart, &start );
	StateToVec( stateEnd, &end );

	float dx = (float)(start.x-end.x);
	float dy = (float)(start.y-end.y);

	return sqrtf( dx*dx + dy*dy );
}
*/


/*
Map::QuadTree::QuadTree()
{
	Clear();
	filterModel = 0;

	int base = 0;
	for( int i=0; i<QUAD_DEPTH+1; ++i ) {
		depthBase[i] = base;
		base += (1<<i)*(1<<i);
	}
}


void Map::QuadTree::Clear()
{
	memset( tree, 0, sizeof(MapItem*)*NUM_QUAD_NODES );
	memset( depthUse, 0, sizeof(int)*(QUAD_DEPTH+1) );
}


void Map::QuadTree::Add( MapItem* item )
{
	int d=0;
	int i = CalcNode( item->mapBounds8, &d );
	item->nextQuad = tree[i];
	tree[i] = item;
	depthUse[d] += 1;

//	GLOUTPUT(( "QuadTree::Add %x id=%d (%d,%d)-(%d,%d) at depth=%d\n", item, item->itemDefIndex,
//				item->mapBounds8.min.x, item->mapBounds8.min.y, item->mapBounds8.max.x, item->mapBounds8.max.y,
//				d ));

	//GLOUTPUT(( "Depth: %2d %2d %2d %2d %2d\n", 
	//		   depthUse[0], depthUse[1], depthUse[2], depthUse[3], depthUse[4] ));
}


void Map::QuadTree::UnlinkItem( MapItem* item )
{
	int d=0;
	int index = CalcNode( item->mapBounds8, &d );
	depthUse[d] -= 1;
	GLRELASSERT( tree[index] ); // the item should be in the linked list somewhere.

//	GLOUTPUT(( "QuadTree::UnlinkItem %x id=%d\n", item, item->itemDefIndex ));

	MapItem* prev = 0;
	for( MapItem* p=tree[index]; p; prev=p, p=p->nextQuad ) {
		if ( p == item ) {
			if ( prev )
				prev->nextQuad = p->nextQuad;
			else
				tree[index] = p->nextQuad;
			return;
		}
	}
	GLASSERT( 0 );	// should have found the item.
}


Map::MapItem* Map::QuadTree::FindItems( const Rectangle2I& bounds, int required, int excluded )
{
	//GRINLIZ_PERFTRACK
	// Walk the map and pull out items in bounds.
	MapItem* root = 0;

	GLASSERT( bounds.min.x >= 0 && bounds.max.x < 256 );	// using int8
	GLASSERT( bounds.min.y >= 0 && bounds.max.y < 256 );	// using int8
	GLASSERT( bounds.min.x >= 0 && bounds.max.x < EL_MAP_SIZE );
	GLASSERT( bounds.min.y >= 0 && bounds.max.y < EL_MAP_SIZE );	// using int8
	Rectangle2<U8> bounds8;
	bounds8.Set( bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y );

	for( int depth=0; depth<QUAD_DEPTH; ++depth ) 
	{
		// Translate into coordinates for this node level:
		int x0 = WorldToNode( bounds.min.x, depth );
		int x1 = WorldToNode( bounds.max.x, depth );
		int y0 = WorldToNode( bounds.min.y, depth );
		int y1 = WorldToNode( bounds.max.y, depth );

		// i, j, x0.. all in node coordinates.
		for( int j=y0; j<=y1; ++j ) {
			for( int i=x0; i<=x1; ++i ) {
				MapItem* pItem = *(tree + depthBase[depth] + NodeOffset( i, j, depth ) );

				if ( filterModel ) {
					while( pItem ) {
						if ( pItem->model == filterModel ) {
							pItem->next = 0;
							return pItem;
						}
						pItem = pItem->nextQuad;
					}
				}
				else {
					while( pItem ) { 
						if (    ( ( pItem->flags & required) == required )
							 && ( ( pItem->flags & excluded ) == 0 )
							 && pItem->mapBounds8.Intersect( bounds8 ) )
						{
							pItem->next = root;
							root = pItem;
						}
						pItem = pItem->nextQuad;
					}
				}
			}
		}
	}
	return root;
}


Map::MapItem* Map::QuadTree::FindItem( const Model* model )
{
	GLRELASSERT( model->IsFlagSet( Model::MODEL_OWNED_BY_MAP ) );
	Rectangle2I b;

	// May or may not have 'mapBoundsCache' when called.
//	if ( model->mapBoundsCache.min.x >= 0 ) {
//		b = model->mapBoundsCache;
//	}
//	else {
		// Need a little space around the coordinates to account
		// for object rotation.
		b.min.x = Clamp( (int)model->X()-2, 0, SIZE-1 );
		b.min.y = Clamp( (int)model->Z()-2, 0, SIZE-1 );
		b.max.x = Clamp( (int)model->X()+2, 0, SIZE-1 );
		b.max.y = Clamp( (int)model->Z()+2, 0, SIZE-1 );
//	}
	filterModel = model;
	MapItem* root = FindItems( b, 0, 0 );
	filterModel = 0;
	GLRELASSERT( root && root->next == 0 );
	return root;
}


int Map::QuadTree::CalcNode( const Rectangle2<U8>& bounds, int* d )
{
	int offset = 0;

	for( int depth=QUAD_DEPTH-1; depth>0; --depth ) 
	{
		int x0 = WorldToNode( bounds.min.x, depth );
		int y0 = WorldToNode( bounds.min.y, depth );

		int wSize = SIZE >> depth;

		Rectangle2<U8> wBounds;
		wBounds.Set(	NodeToWorld( x0, depth ),
						NodeToWorld( y0, depth ),
						NodeToWorld( x0, depth ) + wSize - 1,
						NodeToWorld( y0, depth ) + wSize - 1 );

		if ( wBounds.Contains( bounds ) ) {
			offset = depthBase[depth] + NodeOffset( x0, y0, depth );
			if ( d )
				*d = depth;
			break;
		}
	}
	GLRELASSERT( offset >= 0 && offset < NUM_QUAD_NODES );
	return offset;
}


void Map::QuadTree::MarkVisible( const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>& fogOfWar )
{
	Rectangle2I mapBounds;
	mapBounds.Set( 0, 0, SIZE-1, SIZE-1 );
	MapItem* root = FindItems( mapBounds, 0, 0 );

	for( MapItem* item=root; item; item=item->next ) {
		
		Rectangle2I b = item->MapBounds();
		if ( b.min.x > 0 ) b.min.x--;
		if ( b.max.x < SIZE-1 ) b.max.x++;
		if ( b.min.y > 0 ) b.min.y--;
		if ( b.max.y < SIZE-1 ) b.max.y++;

		if ( fogOfWar.IsRectEmpty( b ) ) {
			if ( item->model ) item->model->SetFlag( Model::MODEL_INVISIBLE );
		}
		else {
			if ( item->model ) item->model->ClearFlag( Model::MODEL_INVISIBLE );
		}
	}
}
*/



void Map::CreateTexture( Texture* t )
{
	GLASSERT( 0 );
}


void Map::BeginRender()
{
	gamuiShader.PushMatrix( GPUShader::MODELVIEW_MATRIX );

	Matrix4 rot;
	rot.SetXRotation( 90 );
	gamuiShader.MultMatrix( GPUShader::MODELVIEW_MATRIX, rot );
}


void Map::EndRender()
{
	gamuiShader.PopMatrix( GPUShader::MODELVIEW_MATRIX );
}


void Map::BeginRenderState( const void* renderState )
{
	const float ALPHA = 0.5f;
	const float ALPHA_1 = 0.30f;
	switch( (int)renderState ) {
		case UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE:
		case RENDERSTATE_MAP_OPAQUE:
			gamuiShader.SetColor( 1, 1, 1, 1 );
			gamuiShader.SetBlend( false );
			break;

		case UIRenderer::RENDERSTATE_UI_NORMAL:
		case RENDERSTATE_MAP_NORMAL:
			gamuiShader.SetColor( 1.0f, 1.0f, 1.0f, 0.8f );
			gamuiShader.SetBlend( true );
			break;

		case RENDERSTATE_MAP_TRANSLUCENT:
			gamuiShader.SetColor( 1, 1, 1, ALPHA );
			gamuiShader.SetBlend( true );
			break;
		case RENDERSTATE_MAP_MORE_TRANSLUCENT:
			gamuiShader.SetColor( 1, 1, 1, ALPHA_1 );
			gamuiShader.SetBlend( true );
			break;
		default:
			GLASSERT( 0 );
	}
}


void Map::BeginTexture( const void* textureHandle )
{
	gamuiShader.SetTexture0( (Texture*)textureHandle );
}


void Map::Render( const void* renderState, const void* textureHandle, int nIndex, const uint16_t* index, int nVertex, const gamui::Gamui::Vertex* vertex )
{
	GPUStream stream( GPUStream::kGamuiType );
	gamuiShader.SetStream( stream, vertex, nIndex, index );

	gamuiShader.Draw();
}
