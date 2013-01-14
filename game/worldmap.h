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
	Remembering Y is up and we are in the xz plane:

	+--------x
	|
	|
	|
	|
	z

	1. The (0,0) world origin is in the upper left.
	2. This is annoying, but it's important to keep sane.
	3. If the mark is used it should display, save, and load in the upper left
*/

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

	void SetBlocked( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlocked( pos ); }
	void SetBlocked( const grinliz::Rectangle2I& pos );
	void ClearBlocked( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); ClearBlocked( pos ); }
	void ClearBlocked( const grinliz::Rectangle2I& pos );

	/*
	bool IsBlocked( int x, int y )	{ 
		int i = INDEX(x,y);
		return WorldGridState::IsBlocked( grid[i], gridState[i] ); 
	}
	bool IsLand( int x, int y )		{ 
		int i = INDEX(x,y);
		return grid[i].IsLand(); 
	}
	*/
	
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

	grinliz::Vector2I ZoneOrigin( int x, int y ) const {
		const WorldGridState& gs = gridState[INDEX(x,y)];
		grinliz::Vector2I origin = gs.ZoneOrigin( x, y );
		return origin;
	}

	const WorldGrid& ZoneOriginG( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return grid[INDEX(v)];
	}

	WorldGridState* ZoneOriginGS( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return gridState + INDEX(v);
	}

	bool IsZoneOrigin( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return v.x == x && v.y == y;
	}

	bool IsPassable( int x, int y ) const {
		int index = INDEX(x,y);
		return WorldGridState::IsPassable( grid[index], gridState[index] );
	}

	grinliz::Rectangle2F ZoneBounds( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		const WorldGridState& gs = gridState[INDEX(x,y)];
		grinliz::Rectangle2F b;
		b.min.Set( (float)v.x, (float)v.y );
		int size = gs.ZoneSize();
		b.max.Set( (float)(v.x + size), (float)(v.y + size) );
		return b;
	}

	grinliz::Vector2F ZoneCenter( int x, int y ) const {
		grinliz::Rectangle2F r = ZoneBounds( x, y );
		return r.Center();
	}

	void* ToState( int x, int y ) {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return (void*)(v.y*width+v.x);
	}

	WorldGrid* ToGrid( void* state, grinliz::Vector2I* vec, WorldGridState** gs=0 ) {
		int v = (int)(state);
		if ( vec ) {
			int y = v / width;
			int x = v - y*width;
			vec->Set( x, y );
		}
		GLASSERT( v < width*height );
		if ( gs ) {
			*gs = gridState+v;
		}
		return grid+v;
	}

	WorldGrid*					grid;
	WorldGridState*				gridState;

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
