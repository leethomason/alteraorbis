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

#ifndef LUMOS_WORLD_MAP_INCLUDED
#define LUMOS_WORLD_MAP_INCLUDED

#include "gamelimits.h"
#include "worldgrid.h"

#include "../engine/map.h"
#include "../engine/rendertarget.h"

#include "../micropather/micropather.h"

#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glmemorypool.h"
#include "../grinliz/glbitarray.h"

#include "../tinyxml2/tinyxml2.h"

class Texture;
class WorldInfo;
struct WorldFeature;

/*
	The world map has layers. (May add more in the future.)
	Basic layer: water or land.
	For land, is it passable or not?
	In the future, flowing water, magma, etc. would be great.

	Rendering is currently broken into strips. (Since the map
	doesn't change after the Init...() is called.) Could be
	further optimized to use regions.
*/
class WorldMap : public Map, 
	             public micropather::Graph
{
public:
	WorldMap( int width, int height );
	~WorldMap();

	// Test initiliazation:
	void InitCircle();
	bool InitPNG( const char* filename, 
				  grinliz::CDynArray<grinliz::Vector2I>* blocks,
				  grinliz::CDynArray<grinliz::Vector2I>* wayPoints,
				  grinliz::CDynArray<grinliz::Vector2I>* features );
	// Init from WorldGen data:
	void Init( const U8* land, const U8* color, grinliz::CDynArray< WorldFeature >& featureArr );

	void Save( const char* pathToPNG, const char* pathToXML );
	void Load( const char* pathToPNG, const char* pathToXML );

	void SetBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void SetBlock( const grinliz::Rectangle2I& pos );
	void ClearBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); ClearBlock( pos ); }
	void ClearBlock( const grinliz::Rectangle2I& pos );

	bool IsBlockSet( int x, int y ) { return grid[INDEX(x,y)].IsBlock(); }
	bool IsLand( int x, int y )		{ return grid[INDEX(x,y)].IsLand(); }
	
	// Call the pather; return true if successful.
	bool CalcPath(	const grinliz::Vector2F& start, 
					const grinliz::Vector2F& end, 
					grinliz::CDynArray<grinliz::Vector2F> *path,
					float* totalCost,
					bool showDebugging = false );
	bool CalcPath(	const grinliz::Vector2F& start, 
					const grinliz::Vector2F& end, 
					grinliz::Vector2F *path,
					int *pathLen,
					int maxPath,
					float* totalCost,
					bool showDebugging = false );

	enum BlockResult {
		NO_EFFECT,
		FORCE_APPLIED,
		STUCK
	};

	// Calculate the effect of walls on 'pos'. Note that
	// there can be multiple walls, and this takes multiple calls.
	BlockResult CalcBlockEffect(	const grinliz::Vector2F& pos,
									float radius,
									grinliz::Vector2F* force );
	// Call CalcBlockEffect and return the result.
	BlockResult ApplyBlockEffect(	const grinliz::Vector2F inPos, 
									float radius, 
									grinliz::Vector2F* outPos );

	// ---- Map ---- //
	virtual void Submit( GPUState* shader, bool emissiveOnly );
	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUState::StencilMode );

	// ---- MicroPather ---- //
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	// --- Debugging -- //
	void ShowAdjacentRegions( float x, float y );
	void ShowRegionPath( float x0, float y0, float x1, float y1 );
	void ShowRegionOverlay( bool over ) { debugRegionOverlay = over; }
	void PatherCacheHitMiss( micropather::CacheData* data )	{ pather->GetCacheData( data ); }
	int CalcNumRegions();

	// --- MetaData --- //
	const WorldInfo& GetWorldInfo()		{ return *worldInfo; }
	// Find random land on the largest continent
	grinliz::Vector2I FindEmbark();
	const grinliz::Color4U8* Pixels()	{ return (const grinliz::Color4U8*) grid; }

private:
	int INDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height ); 
		return y*width + x; 
	}
	int INDEX( grinliz::Vector2I v ) const { return INDEX( v.x, v.y ); }

	int ZDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height );
		x /= ZONE_SIZE;
		y /= ZONE_SIZE;
		return (y*width/ZONE_SIZE) + x; 
	} 

	void Init( int w, int h );
	void Tessellate();
	void CalcZone( int x, int y );

	bool DeleteRegion( int x, int y );	// surprisingly complex
	void DeleteAllRegions();

	void DrawZones();			// debugging
	void ClearDebugDrawing();	// debugging
	void DumpRegions();

	/* A 16x16 zone, needs 3 bits to describe the depth. From the depth
	   can infer the origin, etc.
		d=0 16
		d=1 8
		d=2 4
		d=3 2
		d=4 1
	*/
	enum {
		TRUE		= 1,
		FALSE		= 0,
		ZONE_SIZE	= 16,		// adjust size of bit fields to be big enough to represent this
		ZONE_SHIFT  = 4,
		ZONE_SIZE2  = ZONE_SIZE*ZONE_SIZE,
	};
	

	// The solver has 3 components:
	//	Vector path:	the final result, a collection of points that form connected vector
	//					line segments.
	//	Grid path:		intermediate; a path checked by a step walk between points on the grid
	//  State path:		the micropather computed region

	bool GridPath( const grinliz::Vector2F& start, const grinliz::Vector2F& end );

	WorldGrid* GridOrigin( int x, int y ) {
		const WorldGrid& g = grid[INDEX(x,y)];
		U32 mask = 0xffffffff;
		U32 size = g.ZoneSize();
		if ( size ) {
			mask = ~(size-1);
		}
		return grid + INDEX( x&mask, y&mask );
	}

	grinliz::Vector2I RegionOrigin( int x, int y ) {
		const WorldGrid& g = grid[INDEX(x,y)];
		U32 mask = 0xffffffff;
		U32 size = g.ZoneSize();
		if ( size ) {
			mask = ~(size-1);
		}
		grinliz::Vector2I v = { x&mask, y&mask };
		return v;
	}

	bool IsRegionOrigin( int x, int y ) { 
		grinliz::Vector2I v = RegionOrigin( x, y );
		return (x == v.x && y == v.y);
	}

	grinliz::Vector2F RegionCenter( int x, int y ) {
		const WorldGrid& g = grid[INDEX(x,y)];
		grinliz::Vector2I v = RegionOrigin( x, y );
		float half = (float)g.ZoneSize() * 0.5f;
		grinliz::Vector2F c = { (float)(x) + half, (float)y + half };
		return c;
	}

	grinliz::Rectangle2F RegionBounds( int x, int y ) {
		const WorldGrid& g = grid[INDEX(x,y)];
		grinliz::Vector2I v = RegionOrigin( x, y );
		grinliz::Rectangle2F b;
		b.min.Set( (float)v.x, (float)v.y );
		b.max.Set( (float)(v.x + g.ZoneSize()), (float)(v.y + g.ZoneSize()) );
		return b;
	}
		
	void* ToState( int x, int y ) {
		grinliz::Vector2I v = RegionOrigin( x, y );
		return (void*)(v.y*width+v.x);
	}

	WorldGrid* ToGrid( void* state, grinliz::Vector2I* vec ) {
		int v = (int)(state);
		if ( vec ) {
			int y = v / width;
			int x = v - y*width;
			vec->Set( x, y );
		}
		GLASSERT( v < width*height );
		return grid+v;
	}

	WorldGrid*					grid;		// pathing info.
	WorldInfo*					worldInfo;
	micropather::MicroPather*	pather;
	bool						debugRegionOverlay;

	MP_VECTOR< void* >						pathRegions;
	grinliz::CDynArray< grinliz::Vector2F >	debugPathVector;
	grinliz::CDynArray< grinliz::Vector2F >	pathCache;

	enum {
		LOWER_TYPES = 2		// land or water
	};
	Texture*						texture[LOWER_TYPES];
	grinliz::CDynArray<PTVertex>	vertex[LOWER_TYPES];
	grinliz::CDynArray<U16>			index[LOWER_TYPES];
	grinliz::BitArray< MAX_MAP_SIZE/ZONE_SIZE, MAX_MAP_SIZE/ZONE_SIZE, 1 > zoneInit;
};

#endif // LUMOS_WORLD_MAP_INCLUDED
