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
class WorldMap : public Map 
	             //public micropather::Graph
{
public:
	WorldMap( int width, int height );
	~WorldMap();

	void InitCircle();

	void SetBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void SetBlock( const grinliz::Rectangle2I& pos );
	void ClearBlock( int x, int y )	{ grinliz::Rectangle2I pos; pos.Set( x, y, x, y ); SetBlock( pos ); }
	void ClearBlock( const grinliz::Rectangle2I& pos );

	bool IsBlockSet( int x, int y ) { return (grid[INDEX(x,y)] & BLOCK) != 0; }
	bool IsLand( int x, int y )		{ return (grid[INDEX(x,y)] & LAND) != 0; }

	// ---- Map ---- //
	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode );

	// ---- MicroPather ---- //
//	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
//	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
//	virtual void  PrintStateInfo( void* state );

private:
	int INDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height ); 
		return y*width + x; 
	}
	int ZDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height );
		x /= ZONE_SIZE;
		y /= ZONE_SIZE;
		return (y*width/ZONE_SIZE) + x; 
	} 
	void Tessellate();
	void CalcZone( int x, int y );
	void CalcZoneRec( int x, int y, int depth );
	
	// Debugging
	void DrawZones();


	enum {
		LAND		= 0x01,		// tessellator depends on this being one; if not, fix tess
		BLOCK		= 0x02,
		LOW_MASK    = LAND | BLOCK,
		DEPTH_MASK  = 0x1c,
		DEPTH_SHIFT = 2,

		ZONE_SIZE	= 16,
		ZONE_SIZE2  = ZONE_SIZE*ZONE_SIZE,
		ZONE_DEPTHS = 5
	};
	static int GridPassable( U8 g ) {
		return (g & LOW_MASK) == LAND;
	}
	static int GridSetDepth( U8 g, int depth ) {
		GLASSERT( depth >= 0 && depth < ZONE_DEPTHS );
		return (g & LOW_MASK) | (depth<<DEPTH_SHIFT);
	}
	static void ZoneGet( int x, int y, int g, int* zx, int* zy, int* size ) {
		if ( !GridPassable( g ) ) {
			*zx = x; *zy = y; *size = 0;
		}
		else {
			int depth = (g&DEPTH_MASK)>>DEPTH_SHIFT;
			*size = ZoneDepthToSize( depth );
			int s = *size - 1;
			*zx = x & (~s);
			*zy = y & (~s);
		}
	}

	// 16-8-4-2-1 
	static int ZoneDepthToSize( int depth ) {
		GLASSERT( depth >= 0 && depth < ZONE_DEPTHS );
		return ZONE_SIZE >> depth;
	}

	// 2 bits flags.
	// 3 bits depth.
	// 2 bits pad.
	U8* grid;		// pathing info.
	U8* zoneInit;	// flag whether this zone is valid.

	enum {
		LOWER_TYPES = 2		// land or water
	};
	Texture*			texture[LOWER_TYPES];
	CDynArray<PTVertex> vertex[LOWER_TYPES];
	CDynArray<U16>		index[LOWER_TYPES];
};

#endif // LUMOS_WORLD_MAP_INCLUDED
