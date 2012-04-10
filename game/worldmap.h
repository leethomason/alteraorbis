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

	How much of the map can be rendered at once? Efficiently?
	Normally not much, but it is nice to be able to zoom and do
	full world renders. Could actually do it all on the shader,
	but that causes a dependent texture read. (And is tricky.)
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

	// ---- Map ---- //
	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode );

	// ---- MicroPather ---- //
//	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
//	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
//	virtual void  PrintStateInfo( void* state );

private:
	int INDEX( int x, int y ) const { GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height ); return y*width + x; }
	void Tessellate();
	
	enum {
		LAND  = 0x01,		// tessellator depends on this being one; if not, fix tess
		BLOCK = 0x02,

		LOWER_TYPES = 2		// land or water
	};
	U8* grid;			// what is set and blocked from pathing. [1 Meg]

	Texture*			texture[LOWER_TYPES];
	CDynArray<PTVertex> vertex[LOWER_TYPES];
	CDynArray<U16>		index[LOWER_TYPES];
};

#endif // LUMOS_WORLD_MAP_INCLUDED
