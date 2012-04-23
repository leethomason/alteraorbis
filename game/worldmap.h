#ifndef LUMOS_WORLD_MAP_INCLUDED
#define LUMOS_WORLD_MAP_INCLUDED

#include "../engine/map.h"
#include "../micropather/micropather.h"
#include "../grinliz/glrectangle.h"
#include "../engine/ufoutil.h"

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

	void SetBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void SetBlock( const grinliz::Rectangle2I& pos );
	void ClearBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void ClearBlock( const grinliz::Rectangle2I& pos );

	bool IsBlockSet( int x, int y ) { return grid[INDEX(x,y)].isBlock != 0; }
	bool IsLand( int x, int y )		{ return grid[INDEX(x,y)].isLand != 0; }
	
	// Call the pather; return true if successful.
	bool CalcPath(	const grinliz::Vector2F& start, 
					const grinliz::Vector2F& end, 
					CDynArray<grinliz::Vector2F> *path,
					bool showDebugging = false );
	bool CalcPath(	const grinliz::Vector2F& start, 
					const grinliz::Vector2F& end, 
					grinliz::Vector2F *path,
					int *pathLen,
					int maxPath,
					bool showDebugging = false );

	// ---- Map ---- //
	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode );

	// ---- MicroPather ---- //
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	// --- Debugging -- //
	void ShowAdjacentRegions( float x, float y );
	void ShowRegionPath( float x0, float y0, float x1, float y1 );
	int NumRegions() const;

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

	void Tessellate();
	void CalcZone( int x, int y );

	// The solver has 3 components:
	//	Vector path:	the final result, a collection of points that form connected vector
	//					line segments.
	//	Grid path:		intermediate; a path checked by a step walk between points on the grid
	//  Region path:	the micropather computed region

	// Call the region solver. Put the result in the pathVector
	int  RegionSolve( const grinliz::Vector2I& subZoneStart, const grinliz::Vector2I& subZoneEnd );
	bool GridPath( const grinliz::Vector2F& start, const grinliz::Vector2F& end );

	void DrawZones();			// debugging
	void ClearDebugDrawing();	// debugging

	enum {
		TRUE = 1,
		FALSE = 0,
		ZONE_SIZE	= 16,		// adjust size of bit fields to be big enough to represent this
		ZONE_SIZE2  = ZONE_SIZE*ZONE_SIZE,
	};

	// FIXME: many sites claim bitfields defeat the optimizer,
	// and it may be possible to pack into 16 bits. Makes for
	// way cleaner code though.
	struct Grid {
		U32 isLand			: 1;
		U32 isBlock			: 1;
		U32 isPathInit		: 1;	// true if the sub-zone is computed
		U32 deltaXOrigin	: 4;	// interpreted as negative
		U32 deltaYOrigin	: 4;
		U32 sizeX			: 5;
		U32 sizeY			: 5;

		U32 debug_origin	: 1;
		U32 debug_adjacent	: 1;
		U32 debug_path		: 1;

		bool IsPassable() const			{ return isLand == TRUE && isBlock == FALSE; }
		bool IsRegionOrigin() const		{ return IsPassable() && deltaXOrigin == 0 && deltaYOrigin == 0; }
		void SetPathOrigin( int dx, int dy, int sizex, int sizey ) {
			GLASSERT( dx >= 0 && dx < ZONE_SIZE );
			GLASSERT( dy >= 0 && dy < ZONE_SIZE );
			GLASSERT( sizex > 0 && sizex <= ZONE_SIZE );
			GLASSERT( sizey > 0 && sizey <= ZONE_SIZE );
			GLASSERT( IsPassable() );
			deltaXOrigin = dx;
			deltaYOrigin = dy;
			sizeX = sizex;
			sizeY = sizey;
			isPathInit = true;
		}
		void CalcBounds( float x, float y, grinliz::Rectangle2F* b ) {
			b->Set( x, y, x+(float)sizeX, y+(float)sizeY );
		}
	};

	// Returns the location of the sub-zone, (-1,-1) for DNE
	grinliz::Vector2I GetRegion( int x, int y ) {
		Grid g = grid[INDEX(x,y)];
		grinliz::Vector2I v = { -1, -1 };
		if ( g.IsPassable() ) {
			v.Set( x-g.deltaXOrigin, y-g.deltaYOrigin );
		}
		return v;
	}

	void* ToState( int x, int y ) {
		GLASSERT( x >= 0 && x < width && y >= 0 && y < height );
		GLASSERT( grid[INDEX(x,y)].deltaXOrigin == 0 && grid[INDEX(x,y)].deltaYOrigin == 0 );
		return reinterpret_cast<void*>( x | (y<<16) );
	}
	Grid ToGrid( void* state, int* x, int *y ) {
		int v32 = reinterpret_cast<int>( state );
		*x = v32 & 0xffff;
		*y = (v32 & 0xffff0000)>>16;
		return grid[INDEX(*x,*y)];
	}

	bool JointPassable( int x0, int y0, int x1, int y1 );
	bool RectPassable( int x0, int y0, int x1, int y1 );

	Grid* grid;		// pathing info.
	U8* zoneInit;	// flag whether this zone is valid.
	micropather::MicroPather *pather;

	MP_VECTOR< void* >				pathRegions;
	CDynArray< grinliz::Vector2F >	debugPathVector;
	CDynArray< grinliz::Vector2F >	pathCache;

	enum {
		LOWER_TYPES = 2		// land or water
	};
	Texture*			texture[LOWER_TYPES];
	CDynArray<PTVertex> vertex[LOWER_TYPES];
	CDynArray<U16>		index[LOWER_TYPES];
};

#endif // LUMOS_WORLD_MAP_INCLUDED
