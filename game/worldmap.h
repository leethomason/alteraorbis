#ifndef LUMOS_WORLD_MAP_INCLUDED
#define LUMOS_WORLD_MAP_INCLUDED

#include "../engine/map.h"
#include "../engine/rendertarget.h"
#include "../micropather/micropather.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glmemorypool.h"

class Texture;

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

	void InitCircle();
	bool InitPNG( const char* filename, 
				  grinliz::CDynArray<grinliz::Vector2I>* blocks,
				  grinliz::CDynArray<grinliz::Vector2I>* wayPoints,
				  grinliz::CDynArray<grinliz::Vector2I>* features );

	void SetBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void SetBlock( const grinliz::Rectangle2I& pos );
	void ClearBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); ClearBlock( pos ); }
	void ClearBlock( const grinliz::Rectangle2I& pos );

	bool IsBlockSet( int x, int y ) { return grid[INDEX(x,y)].isBlock != 0; }
	bool IsLand( int x, int y )		{ return grid[INDEX(x,y)].isLand != 0; }
	
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
	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode );

	// ---- MicroPather ---- //
	virtual float LeastCostEstimate( micropather::PathNode* stateStart, micropather::PathNode* stateEnd );
	virtual void AdjacentCost( micropather::PathNode* state, micropather::StateCost** adjacent, int* numAdjacent );
	virtual void  PrintStateInfo( micropather::PathNode* state );

	// --- Debugging -- //
	void ShowAdjacentRegions( float x, float y );
	void ShowRegionPath( float x0, float y0, float x1, float y1 );
	void ShowRegionOverlay( bool over ) { debugRegionOverlay = over; }
	float PatherCache();
	void PatherCacheHitMiss( int* hits, int* miss, float* ratio ) { pather->PathCacheHitMiss( hits, miss, ratio ); }

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

	enum {
		TRUE		= 1,
		FALSE		= 0,
		ZONE_SIZE	= 16,		// adjust size of bit fields to be big enough to represent this
		ZONE_SIZE2  = ZONE_SIZE*ZONE_SIZE,
	};
	
	/* A rectangular area used for pathing. 
	   Keeps information about location and size.
	   Note that the 'adjacent' is not symmetric.
	   The adjacent is created on demand, so neighbors
	   may not have the array cached.
	*/
	struct Region : public micropather::PathNode
	{
		U8 debug_origin		: 1;
		U8 debug_adjacent	: 1;
		U8 debug_path		: 1;

		U16 x, y, dx, dy;
		grinliz::CDynArray< micropather::StateCost> adjacent;

		Region() : x(-1), y(-1), dx(-1), dy(-1), debug_origin(0), debug_adjacent(0), debug_path(0) {}
		~Region() {
			// MUST BE UNLINKED!
			GLASSERT( adjacent.Size() == 0 );
		}

		void Init( U16 _x, U16 _y, U16 _dx, U16 _dy ) {
			x = _x; y = _y; dx = _dx; dy = _dy;
		}
		grinliz::Vector2I OriginI() const {
			GLASSERT( x >= 0 && y >= 0 && dx > 0 && dy > 0 );
			grinliz::Vector2I v = { x, y };
			return v;
		}

		grinliz::Vector2F CenterF() const {
			GLASSERT( x >= 0 && y >= 0 && dx > 0 && dy > 0 );
			grinliz::Vector2F v = { (float)x + (float)dx*0.5f, (float)y + (float)dy*0.5f };
			return v;
		}

		void BoundsF( grinliz::Rectangle2F* r ) const {
			GLASSERT( x >= 0 && y >= 0 && dx > 0 && dy > 0 );
			r->Set( (float)x, (float)y, (float)(x+dx), (float)(y+dy) );
		}
	};
	grinliz::MemoryPoolT<Region> regionPlex;

	// The solver has 3 components:
	//	Vector path:	the final result, a collection of points that form connected vector
	//					line segments.
	//	Grid path:		intermediate; a path checked by a step walk between points on the grid
	//  Region path:	the micropather computed region

	// Call the region solver. Put the result in the pathVector
	int  RegionSolve( Region* start, Region* end, float* totalCost );
	bool GridPath( const grinliz::Vector2F& start, const grinliz::Vector2F& end );

	// FIXME: many sites claim bitfields defeat the optimizer,
	// and it may be possible to pack into 16 bits. Makes for
	// way cleaner code though.
	struct Grid {
		U32 isLand			: 1;
		U32 isBlock			: 1;
		U32 color			: 8;	// zone color

		Region* region;

		bool IsPassable() const			{ return isLand == TRUE && isBlock == FALSE; }
	};
	bool IsRegionOrigin( const Region* r, int x, int y ) { return r && r->x == x && r->y ==y; }

	micropather::PathNode* ToState( int x, int y ) {
		GLASSERT( x >= 0 && x < width && y >= 0 && y < height );
		Region* r = grid[INDEX(x,y)].region;
		GLASSERT( r );
		return r;
	}
	Region* ToGrid( micropather::PathNode* state ) {
		return static_cast<Region*>(state);
	}

	bool RectPassableAndOpen( int x0, int y0, int x1, int y1 );

	Grid* grid;		// pathing info.
	U8* zoneInit;	// flag whether this zone is valid.
	micropather::MicroPather *pather;
	bool debugRegionOverlay;

	MP_VECTOR< micropather::PathNode* >		pathRegions;
	grinliz::CDynArray< grinliz::Vector2F >	debugPathVector;
	grinliz::CDynArray< grinliz::Vector2F >	pathCache;

	enum {
		LOWER_TYPES = 2		// land or water
	};
	Texture*						texture[LOWER_TYPES];
	grinliz::CDynArray<PTVertex>	vertex[LOWER_TYPES];
	grinliz::CDynArray<U16>			index[LOWER_TYPES];
};

#endif // LUMOS_WORLD_MAP_INCLUDED
