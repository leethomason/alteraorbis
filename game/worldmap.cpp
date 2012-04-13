#include "worldmap.h"
#include "../grinliz/glutil.h"
#include "../engine/texture.h"

using namespace grinliz;

WorldMap::WorldMap( int width, int height ) : Map( width, height )
{
	GLASSERT( width % ZONE_SIZE == 0 );
	GLASSERT( height % ZONE_SIZE == 0 );

	grid = new U8[width*height];
	zoneInit = new U8[width*height/ZONE_SIZE2];
	memset( grid, 0, width*height*sizeof(*grid) );
	memset( zoneInit, 0, sizeof(*zoneInit)*width*height/ZONE_SIZE2 );

	texture[0] = TextureManager::Instance()->GetTexture( "map_water" );
	texture[1] = TextureManager::Instance()->GetTexture( "map_land" );
}


WorldMap::~WorldMap()
{
	delete [] grid;
	delete [] zoneInit;
}


void WorldMap::InitCircle()
{
	memset( grid, 0, width*height*sizeof(*grid) );
	memset( zoneInit, 0, sizeof(*zoneInit)*width*height/ZONE_SIZE2 );

	const int R = Min( width, height )/2;
	const int R2 = R * R;
	const int cx = width/2;
	const int cy = height/2;

	for( int y=0; y<height; ++y ) {
		for( int x=0; x<width; ++x ) {
			int r2 = (x-cx)*(x-cx) + (y-cy)*(y-cy);
			if ( r2 < R2 ) {
				int i = INDEX( x, y );
				grid[i] = LAND;
			}
		}
	}
	Tessellate();
}


void WorldMap::Tessellate()
{
	for( int i=0; i<LOWER_TYPES; ++i ) {
		vertex[i].Clear();
		index[i].Clear();
	}
	// Could be much fancier: instead of just strips,
	// expand into 2 directions.
	//
	// 1   3
	// 0   2
	for( int j=0; j<height; ++j ) {
		int i=0;
		while ( i < width ) {
			int layer = ( grid[INDEX(i,j)] & LAND );

			U16* pi = index[layer].PushArr( 6 );
			int base = vertex[layer].Size();
			pi[0] = base;
			pi[1] = base+3;
			pi[2] = base+2;
			pi[3] = base;
			pi[4] = base+1;
			pi[5] = base+3;

			PTVertex* pv = vertex[layer].PushArr( 2 );
			pv[0].pos.Set( (float)i, 0, (float)j );
			pv[0].tex.Set( 0, 0 );
			pv[1].pos.Set( (float)i, 0, (float)(j+1) );
			pv[1].tex.Set( 0, 1 );

			int w = 1;
			++i;
			while( i<width && ((grid[INDEX(i,j)]&LAND) == layer) ) {
				++i;
				++w;
			}
			pv = vertex[layer].PushArr( 2 );
			pv[0].pos.Set( (float)i, 0, (float)j );
			pv[0].tex.Set( (float)w, 0 );
			pv[1].pos.Set( (float)i, 0, (float)(j+1) );
			pv[1].tex.Set( (float)w, 1 );
		}
	}
}


void WorldMap::SetBlock( const grinliz::Rectangle2I& pos )
{
	for( int y=pos.min.y; y<=pos.max.y; ++y ) {
		for( int x=pos.min.x; x<=pos.max.x; ++x ) {
			int i = INDEX( x, y );
			GLASSERT( (grid[i] & BLOCK) == 0 );
			grid[i] |= BLOCK;
			zoneInit[ZDEX( x, y )] = 0;
			GLOUTPUT(( "Block (%d,%d) index=%d zone=%d set\n", x, y, i, ZDEX(x,y) ));
		}
	}
}


void WorldMap::ClearBlock( const grinliz::Rectangle2I& pos )
{
	for( int y=pos.min.y; y<=pos.max.y; ++y ) {
		for( int x=pos.min.x; x<=pos.max.x; ++x ) {
			int i = INDEX( x, y );
			GLASSERT( grid[i] & BLOCK );
			grid[i] &= ~BLOCK;
			zoneInit[ZDEX( x, y )] = 0;
		}
	}
}


void WorldMap::CalcZoneRec( int x, int y, int depth )
{
	int size = ZoneDepthToSize( depth );
	for( int j=y; j<y+size; ++j ) {
		for( int i=x; i<x+size; ++i ) {
			if ( !GridPassable( grid[INDEX(i,j)] )) {
				if ( depth < ZONE_DEPTHS-1 ) {
					CalcZoneRec( x,        y,        depth+1 );
					CalcZoneRec( x+size/2, y,        depth+1 );
					CalcZoneRec( x,        y+size/2, depth+1 );
					CalcZoneRec( x+size/2, y+size/2, depth+1 );
				}
				return;
			}
		}
	}
	// get here then we checked all the zones.
	GLASSERT( depth < ZONE_DEPTHS );
	for( int j=y; j<y+size; ++j ) {
		for( int i=x; i<x+size; ++i ) {
			int index = INDEX(i,j);
			grid[index] = GridSetDepth( grid[index], depth );
		}
	}
}


void WorldMap::CalcZone( int x, int y )
{
	x = x & (~15);
	y = y & (~15);

	if ( zoneInit[ZDEX(x,y)] == 0 ) {
		GLOUTPUT(( "CalcZone (%d,%d) %d\n", x, y, ZDEX(x,y) ));
		CalcZoneRec( x, y, 0 );
		zoneInit[ZDEX(x,y)] = 1;
	}
}


void WorldMap::DrawZones()
{
	CompositingShader debug( GPUShader::BLEND_NORMAL );
	debug.SetColor( 1, 1, 1, 0.5f );

	for( int j=0; j<height; ++j ) {
		for( int i=0; i<width; ++i ) {
			if ( !zoneInit[ZDEX(i,j)] ) {
				CalcZone( i, j );
			}

			int x, y, size;
			static const float offset = 0.2f;
			ZoneGet( i, j, grid[INDEX(i,j)], &x, &y, &size );
			if ( size > 0 && x == i && y == j ) {
				Vector3F p0 = { (float)x+offset, 0.1f, (float)y+offset };
				Vector3F p1 = { (float)(x+size)-offset, 0.1f, (float)(y+size)-offset };
				debug.DrawQuad( p0, p1, false );
			}
		}
	}
}


void WorldMap::Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode mode )
{
	GPUStream stream;

	stream.stride = sizeof( PTVertex );

	stream.posOffset = PTVertex::POS_OFFSET;
	stream.nPos = 3;
	stream.texture0Offset = PTVertex::TEXTURE_OFFSET;
	stream.nTexture0 = 2;

	FlatShader shader;
	shader.SetColor( colorMult.r, colorMult.g, colorMult.b );
	shader.SetStencilMode( mode );
	for( int i=0; i<LOWER_TYPES; ++i ) {
		shader.SetStream( stream, vertex[i].Mem(), index[i].Size(), index[i].Mem() );
		shader.SetTexture0( texture[i] );
		shader.Draw();
	}
	DrawZones();
}

