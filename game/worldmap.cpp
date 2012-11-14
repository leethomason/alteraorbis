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

#include "worldmap.h"

#include "../xegame/xegamelimits.h"

#include "../grinliz/glutil.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glcolor.h"
#include "../shared/lodepng.h"

#include "../engine/texture.h"
#include "../engine/ufoutil.h"

using namespace grinliz;
using namespace micropather;

// Startup for test world
// Baseline:				15,000
// Coloring regions:		 2,300
// Switch to 'struct Region' 2,000
// Region : public PathNode	 1,600
// Bug fix: incorrect recusion   4	yes, 4
// 

WorldMap::WorldMap( int width, int height ) : Map( width, height )
{
	GLASSERT( width % ZONE_SIZE == 0 );
	GLASSERT( height % ZONE_SIZE == 0 );

	grid = 0;
	pather = 0;
	Init( width, height );

	texture[0] = TextureManager::Instance()->GetTexture( "map_water" );
	texture[1] = TextureManager::Instance()->GetTexture( "map_land" );

	debugRegionOverlay = false;
}


WorldMap::~WorldMap()
{
	DeleteAllRegions();
	delete [] grid;
	delete pather;
}


int WorldMap::CalcNumRegions()
{
	int count = 0;
	if ( grid ) {
		// Delete all the regions. Be careful to only
		// delete from the origin location.
		for( int j=0; j<height; ++j ) {	
			for( int i=0; i<width; ++i ) {
				if ( IsRegionOrigin( i, j )) {
					++count;
				}
			}
		}
	}
	return count;
}


void WorldMap::DumpRegions()
{
	if ( grid ) {
		for( int j=0; j<height; ++j ) {	
			for( int i=0; i<width; ++i ) {
				const Grid& g = grid[INDEX(i,j)];
				if ( g.IsPassable() && IsRegionOrigin( i, j )) {
					GLOUTPUT(( "Region %d,%d size=%d", i, j, g.size ));
					GLOUTPUT(( "\n" ));
				}
			}
		}
	}
}


void WorldMap::DeleteAllRegions()
{
	zoneInit.ClearAll();
}


void WorldMap::Init( int w, int h )
{
	DeleteAllRegions();
	delete [] grid;
	if ( pather ) {
		pather->Reset();
	}	
	this->width = w;
	this->height = h;
	grid = new Grid[width*height];
	memset( grid, 0, width*height*sizeof(Grid) );

	delete pather;
	pather = new micropather::MicroPather( this, width*height/4, 8, true );
}


void WorldMap::InitCircle()
{
	memset( grid, 0, width*height*sizeof(Grid) );

	const int R = Min( width, height )/2;
	const int R2 = R * R;
	const int cx = width/2;
	const int cy = height/2;

	for( int y=0; y<height; ++y ) {
		for( int x=0; x<width; ++x ) {
			int r2 = (x-cx)*(x-cx) + (y-cy)*(y-cy);
			if ( r2 < R2 ) {
				int i = INDEX( x, y );
				grid[i].isLand = TRUE;
			}
		}
	}
	Tessellate();
}


bool WorldMap::InitPNG( const char* filename, 
						grinliz::CDynArray<grinliz::Vector2I>* blocks,
						grinliz::CDynArray<grinliz::Vector2I>* wayPoints,
						grinliz::CDynArray<grinliz::Vector2I>* features )
{
	unsigned char* pixels = 0;
	unsigned w=0, h=0;
	static const Color3U8 BLACK = { 0, 0, 0 };
	static const Color3U8 BLUE  = { 0, 0, 255 };
	static const Color3U8 RED   = { 255, 0, 0 };
	static const Color3U8 GREEN = { 0, 255, 0 };

	int error = lodepng_decode24_file( &pixels, &w, &h, filename );
	GLASSERT( error == 0 );
	if ( error == 0 ) {
		Init( w, h );
		int x = 0;
		int y = 0;
		int color=0;
		for( unsigned i=0; i<w*h; ++i ) {
			Color3U8 c = { pixels[i*3+0], pixels[i*3+1], pixels[i*3+2] };
			Vector2I p = { x, y };
			if ( c == BLACK ) {
				grid[i].isLand = 1;
				grid[i].color = color;
				blocks->Push( p );
			}
			else if ( c.r == c.g && c.g == c.b ) {
				grid[i].isLand = 1;
				color = c.r;
				grid[i].color = color;
			}
			else if ( c == BLUE ) {
				grid[i].isLand = 0;
			}
			else if ( c == RED ) {
				grid[i].isLand = 1;
				grid[i].color = color;
				wayPoints->Push( p );
			}
			else if ( c == GREEN ) {
				grid[i].isLand = 1;
				grid[i].color = color;
				features->Push( p );
			}
			++x;
			if ( x == w ) {
				x = 0;
				++y;
			}
		}
		free( pixels );
	}
	Tessellate();
	return error == 0;
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
			int layer = grid[INDEX(i,j)].isLand;

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
			while( i<width && grid[INDEX(i,j)].isLand == layer ) {
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
			GLASSERT( grid[i].isBlock == FALSE );
			grid[i].isBlock = TRUE;
			zoneInit.Clear( x>>ZONE_SHIFT, y>>ZONE_SHIFT );
			//GLOUTPUT(( "Block (%d,%d) index=%d zone=%d set\n", x, y, i, ZDEX(x,y) ));
		}
	}
	pather->Reset();
}


void WorldMap::ClearBlock( const grinliz::Rectangle2I& pos )
{
	for( int y=pos.min.y; y<=pos.max.y; ++y ) {
		for( int x=pos.min.x; x<=pos.max.x; ++x ) {
			int i = INDEX( x, y );
			GLASSERT( grid[i].isBlock );
			grid[i].isBlock = FALSE;
			zoneInit.Clear( x>>ZONE_SHIFT, y>>ZONE_SHIFT );
		}
	}
	pather->Reset();
}


WorldMap::BlockResult WorldMap::CalcBlockEffect(	const grinliz::Vector2F& pos,
													float rad,
													grinliz::Vector2F* force )
{
	Vector2I blockPos = { (int)pos.x, (int)pos.y };

	// could be further optimized by doing a radius-squared check first
	Rectangle2I b = Bounds();
	//static const Vector2I delta[9] = { {0,0}, {-1,-1}, {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0} };
	static const float EPSILON = 0.0001f;

	Vector2I delta[4] = {{ 0, 0 }};
	int nDelta = 1;
	static const float MAX_M1 = 1.0f-MAX_BASE_RADIUS;

	float dx = pos.x - (float)blockPos.x;
	float dy = pos.y - (float)blockPos.y;
	if ( dx < MAX_BASE_RADIUS ) {
		if ( dy < MAX_BASE_RADIUS ) {
			delta[nDelta++].Set( -1,  0 );
			delta[nDelta++].Set( -1, -1 );	
			delta[nDelta++].Set(  0, -1 );
		}
		else if ( dy > MAX_M1 ) {
			delta[nDelta++].Set( -1, 0 );	
			delta[nDelta++].Set( -1, 1 );
			delta[nDelta++].Set(  0, 1 );
		}
		else {
			delta[nDelta++].Set( -1, 0 );
		}
	}
	else if ( dx > MAX_M1 ) {
		if ( dy < MAX_BASE_RADIUS ) {
			delta[nDelta++].Set(  1,  0 );
			delta[nDelta++].Set(  1, -1 );	
			delta[nDelta++].Set(  0, -1 );
		}
		else if ( dy > MAX_M1 ) {
			delta[nDelta++].Set(  1, 0 );	
			delta[nDelta++].Set(  1, 1 );
			delta[nDelta++].Set(  0, 1 );
		}
		else {
			delta[nDelta++].Set( -1, 0 );
		}
	}
	else {
		if ( dy < MAX_BASE_RADIUS ) {
			delta[nDelta++].Set(  0, -1 );
		}
		else if ( dy > MAX_M1 ) {
			delta[nDelta++].Set(  0, 1 );
		}
	}
	GLASSERT( nDelta <= 4 );

	for( int i=0; i<nDelta; ++i ) {
		Vector2I block = blockPos + delta[i];
		if (    b.Contains(block) 
			 && !grid[INDEX(block.x,block.y)].IsPassable() ) 
		{
			// Will find the smallest overlap, and apply.
			Vector2F c = { (float)block.x+0.5f, (float)block.y+0.5f };	// block center.
			Vector2F p = pos - c;										// translate pos to origin
			Vector2F n = p; n.Normalize();

			if ( p.x > fabsf(p.y) && (p.x-rad < 0.5f) ) {				// east quadrant
				float dx = 0.5f - (p.x-rad) + EPSILON;
				GLASSERT( dx > 0 );
				*force = n * (dx/fabsf(n.x));
				return FORCE_APPLIED;
			}
			if ( -p.x > fabsf(p.y) && (p.x+rad > -0.5f) ) {				// west quadrant
				float dx = 0.5f + (p.x+rad) + EPSILON;
				*force = n * (dx/fabsf(n.x));
				GLASSERT( dx > 0 );
				return FORCE_APPLIED;
			}
			if ( p.y > fabsf(p.x) && (p.y-rad < 0.5f) ) {				// north quadrant
				float dy = 0.5f - (p.y-rad) + EPSILON;
				*force = n * (dy/fabsf(n.y));
				GLASSERT( dy > 0 );
				return FORCE_APPLIED;
			}
			if ( -p.y > fabsf(p.x) && (p.y+rad > -0.5f) ) {				// south quadrant
				float dy = 0.5f + (p.y+rad) + EPSILON;
				*force = n * (dy/fabsf(n.y));
				GLASSERT( dy > 0 );
				return FORCE_APPLIED;
			}
		}
	}
	return NO_EFFECT;
}


WorldMap::BlockResult WorldMap::ApplyBlockEffect(	const Vector2F inPos, 
													float radius, 
													Vector2F* outPos )
{
	*outPos = inPos;
	Vector2F force = { 0, 0 };

	// Can't think of a case where it's possible to overlap more than 2,
	// but there probably is. Don't worry about it. Go with fast & 
	// usually good enough.
	for( int i=0; i<2; ++i ) {
		BlockResult result = CalcBlockEffect( *outPos, radius, &force );
		if ( result == STUCK )
			return STUCK;
		if ( result == FORCE_APPLIED )
			*outPos += force;	
		if ( result == NO_EFFECT )
			break;
	}
	return ( *outPos == inPos ) ? NO_EFFECT : FORCE_APPLIED;
}


void WorldMap::CalcZone( int zx, int zy )
{
	zx = zx & (~(ZONE_SIZE-1));
	zy = zy & (~(ZONE_SIZE-1));

	if ( !zoneInit.IsSet( zx>>ZONE_SHIFT, zy>>ZONE_SHIFT) ) {
		//GLOUTPUT(( "CalcZone (%d,%d) %d\n", zx, zy, ZDEX(zx,zy) ));
		zoneInit.Set( zx>>ZONE_SHIFT, zy>>ZONE_SHIFT);

		for( int y=zy; y<zy+ZONE_SIZE; ++y ) {
			for( int x=zx; x<zx+ZONE_SIZE; ++x ) {
				Grid* g = &grid[INDEX(x,y)];
				g->size = g->IsPassable() ? 1 : 0;
			}
		}

		// Could be much more elegant and efficent. Re-walks
		// and re-writes the same data.
		for( int size=2; size<=ZONE_SIZE; size*=2 ) {
			for( int y=zy; y<zy+ZONE_SIZE; y+=size ) {
				for( int x=zx; x<zx+ZONE_SIZE; x+=size ) {
					int half = size >> 1;
					bool okay = true;
					for( int j=y; j<y+size; j+=half ) {
						for( int i=x; i<x+size; i+=half ) {
							Grid* g = &grid[INDEX(i,j)];
							if ( !g->IsPassable() || g->size != half ) {
								okay = false;
								break;
							}
						}
					}
					if ( okay ) {
						for( int j=y; j<y+size; j++ ) {
							for( int i=x; i<x+size; i++ ) {
								Grid* g = &grid[INDEX(i,j)];
								g->size = size;
							}
						}
					}
				}
			}				
		}
	}
}


// micropather
float WorldMap::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	Vector2I startI, endI;
	ToGrid( stateStart, &startI );
	ToGrid( stateEnd, &endI );

	Vector2F start = RegionCenter( startI.x, startI.y );
	Vector2F end   = RegionCenter( endI.x, endI.y );
	return (end-start).Length();
}


// micropather
void WorldMap::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	Vector2I start;
	Grid* startGrid = ToGrid( state, &start );
	GLASSERT( zoneInit.IsSet( start.x>>ZONE_SHIFT, start.y>>ZONE_SHIFT ));
	GLASSERT( startGrid->IsPassable() );
	Vector2F startC = RegionCenter( start.x, start.y );
	
	Rectangle2I bounds = this->Bounds();
	Vector2I currentZone = { -1, -1 };
	CArray< Vector2I, ZONE_SIZE*4+4 > adj;

	int size = startGrid->size;
	GLASSERT( size > 0 );

	// Start and direction for the walk. Note that
	// it needs to be in-order and include diagonals so
	// that adjacent states aren't duplicated.
	const Vector2I borderStart[4] = {
		{ start.x,			start.y-1 },
		{ start.x+size,		start.y },
		{ start.x+size-1,	start.y+size },
		{ start.x-1,		start.y+size-1 }
	};
	static const Vector2I borderDir[4] = {
		{ 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }
	};

	const Vector2I corner[4] = {
		{ start.x+size, start.y-1 }, 
		{ start.x+size, start.y+size },
		{ start.x-1, start.y+size }, 
		{ start.x-1, start.y-1 }, 
	};
	static const Vector2I cornerDir[4] = {
		{-1,1}, {-1,-1}, {1,-1}, {1,1}
	};
	static const Vector2I filter[3] = { {0,0}, {0,1}, {1,0} };


	// Calc all the places that can be adjacent.
	for( int i=0; i<4; ++i ) {
		// Scan the border.
		Vector2I v = borderStart[i];
		for( int k=0; k<size; ++k, v = v + borderDir[i] ) {
			if ( bounds.Contains( v.x, v.y ) )
			{
				Grid* g = grid + INDEX( v.x, v.y );
				if ( g->IsPassable() ) 
				{
					Grid* origin = GridOrigin( v.x, v.y );
					GLASSERT( ToState( v.x, v.y ) != state );
					GLASSERT( origin->IsPassable() );
					adj.Push( v );
				}
			}
		}
		// Check the corner.
		bool pass = true;
		for( int j=0; j<3; ++j ) {
			Vector2I delta = { cornerDir[i].x * filter[j].x, cornerDir[i].y * filter[j].y };
			v = corner[i] + delta;
			if ( bounds.Contains( v ) && grid[INDEX( v.x, v.y )].IsPassable() ) {
				// all is well.
			}
			else {
				pass = false;
				break;
			}
		}
		if ( pass ) {
			GLASSERT( ToState( corner[i].x, corner[i].y ) != state );
			GLASSERT( grid[INDEX(corner[i].x, corner[i].y)].IsPassable() );
			adj.Push( corner[i] );
		}
	}

	Grid* current = 0;
	Grid* first = 0;
	for( int i=0; i<adj.Size(); ++i ) {
		int x = adj[i].x;
		int y = adj[i].y;
		GLASSERT( bounds.Contains( x, y ));

		CalcZone( x, y );
		Grid* g = GridOrigin( x, y );
		GLASSERT( g->IsPassable() );

		// The corners wrap around:
		if ( g == current || g == first)
		{
			continue;
		}
		current = g;
		if ( !first )
			first = g;

		Vector2F otherC = RegionCenter( x, y );
		float cost = (otherC-startC).Length();
		void* s = ToState( x, y );
		GLASSERT( s != state );
		micropather::StateCost sc = { s, cost };
		adjacent->push_back( sc );
	}
}


// micropather
void WorldMap::PrintStateInfo( void* state )
{
	Vector2I vec;
	Grid* g = ToGrid( state, &vec );
	int size = g->size;
	GLOUTPUT(( "(%d,%d)s=%d ", vec.x, vec.y, size ));	
}



// Such a good site, for many years: http://www-cs-students.stanford.edu/~amitp/gameprog.html
// Specifically this link: http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
// Returns true if there is a straight line path between the start and end.
// The line-walk can/should get moved to the utility package. (and the grid lookup replaced with visit() )
bool WorldMap::GridPath( const grinliz::Vector2F& p0, const grinliz::Vector2F& p1 )
{
	double dx = fabs(p1.x - p0.x);
    double dy = fabs(p1.y - p0.y);

    int x = int(floor(p0.x));
    int y = int(floor(p0.y));

    int n = 1;
    int x_inc, y_inc;
    double error;

    if (p1.x > p0.x) {
        x_inc = 1;
        n += int(floor(p1.x)) - x;
        error = (floor(p0.x) + 1 - p0.x) * dy;
    }
    else {
        x_inc = -1;
        n += x - int(floor(p1.x));
        error = (p0.x - floor(p0.x)) * dy;
    }

    if (p1.y > p0.y) {
        y_inc = 1;
        n += int(floor(p1.y)) - y;
        error -= (floor(p0.y) + 1 - p0.y) * dx;
    }
    else {
        y_inc = -1;
        n += y - int(floor(p1.y));
        error -= (p0.y - floor(p0.y)) * dx;
    }

    for (; n > 0; --n)
    {
        if ( !grid[INDEX(x, y)].IsPassable() )
			return false;

        if (error > 0) {
            y += y_inc;
            error -= dx;
        }
        else {
            x += x_inc;
            error += dy;
        }
    }
	return true;
}


bool WorldMap::CalcPath(	const grinliz::Vector2F& start, 
							const grinliz::Vector2F& end, 
							grinliz::Vector2F *path,
							int *len,
							int maxPath,
							float *totalCost,
							bool debugging )
{
	pathCache.Clear();
	bool result = CalcPath( start, end, &pathCache, totalCost, debugging );
	if ( result ) {
		if ( path ) {
			for( int i=0; i<pathCache.Size() && i < maxPath; ++i ) {
				path[i] = pathCache[i];
			}
		}
		if ( len ) {
			*len = Min( maxPath, pathCache.Size() );
		}
	}
	return result;
}


bool WorldMap::CalcPath(	const grinliz::Vector2F& start, 
							const grinliz::Vector2F& end, 
							CDynArray<grinliz::Vector2F> *path,
							float *totalCost,
							bool debugging )
{
	debugPathVector.Clear();
	path->Clear();
	bool okay = false;

	Vector2I starti = { (int)start.x, (int)start.y };
	Vector2I endi   = { (int)end.x,   (int)end.y };

	// Check color:
	U32 c0 = grid[INDEX(starti)].color;
	U32 c1 = grid[INDEX(endi)].color;

	if ( c0 != c1 ) {
		return false;
	}

	// Flush out regions that aren't valid.
	for( int j=0; j<height; j+= ZONE_SIZE ) {
		for( int i=0; i<width; i+=ZONE_SIZE ) {
			CalcZone( i, j );
		}
	}

	Grid* regionStart = grid + INDEX( starti.x, starti.y );
	Grid* regionEnd   = grid + INDEX( endi.x, endi.y );

	GLASSERT( regionStart->IsPassable() && regionEnd->IsPassable() );	// is someone stuck?
	if ( !regionStart->IsPassable() || !regionEnd->IsPassable() ) {
		return false;
	}

	if ( regionStart == regionEnd ) {
		okay = true;
		path->Push( start );
		path->Push( end );
		*totalCost = (end-start).Length();
	}

	// Try a straight line ray cast
	if ( !okay ) {
		okay = GridPath( start, end );
		if ( okay ) {
			//GLOUTPUT(( "Ray succeeded.\n" ));
			path->Push( start );
			path->Push( end );
			*totalCost = (end-start).Length();
		}
	}

	// Use the region solver.
	if ( !okay ) {
		int result = pather->Solve( ToState( starti.x, starti.y ), ToState( endi.x, endi.y ), 
								    &pathRegions, totalCost );
		if ( result == micropather::MicroPather::SOLVED ) {
			//GLOUTPUT(( "Region succeeded len=%d.\n", pathRegions.size() ));
			Vector2F from = start;
			path->Push( start );
			okay = true;
			//Vector2F pos = start;

			// Walk each of the regions, and connect them with vectors.
			for( unsigned i=0; i<pathRegions.size()-1; ++i ) {
				Vector2I originA, originB;
				Grid* rA = ToGrid( pathRegions[i], &originA );
				Grid* rB = ToGrid( pathRegions[i+1], &originB );

				Rectangle2F bA = RegionBounds( originA.x, originA.y );
				Rectangle2F bB = RegionBounds( originB.x, originB.y );					
				bA.DoIntersection( bB );

				// Every point on a path needs to be obtainable,
				// else the chit will get stuck. There inset
				// away from the walls so we don't put points
				// too close to walls to get to.
				static const float INSET = MAX_BASE_RADIUS;
				if ( bA.min.x + INSET*2.0f < bA.max.x ) {
					bA.min.x += INSET;
					bA.max.x -= INSET;
				}
				if ( bA.min.y + INSET*2.0f < bA.max.y ) {
					bA.min.y += INSET;
					bA.max.y -= INSET;
				}

				Vector2F v = bA.min;
				if ( bA.min != bA.max ) {
					int result = ClosestPointOnLine( bA.min, bA.max, from, &v, true );
					GLASSERT( result == INTERSECT );
					if ( result == REJECT ) {
						okay = false;
						break;
					}
				}
				path->Push( v );
				from = v;
			}
			path->Push( end );
		}
	}

	if ( okay ) {
		if ( debugging ) {
			for( int i=0; i<path->Size(); ++i )
				debugPathVector.Push( (*path)[i] );
		}
	}
	else {
		path->Clear();
	}
	return okay;
}


void WorldMap::ClearDebugDrawing()
{
	for( int i=0; i<width*height; ++i ) {
		grid[i].debug_adjacent  = 0;
		grid[i].debug_origin = 0;
		grid[i].debug_path = 0;
	}
	debugPathVector.Clear();
}


void WorldMap::ShowRegionPath( float x0, float y0, float x1, float y1 )
{
	ClearDebugDrawing();

	void* start = ToState( (int)x0, (int)y0 );
	void* end   = ToState( (int)x1, (int)y1 );;
	
	if ( start && end ) {
		float cost=0;
		int result = pather->Solve( start, end, &pathRegions, &cost );
		if ( result == micropather::MicroPather::SOLVED ) {
			for( unsigned i=0; i<pathRegions.size(); ++i ) {
				Grid* vp = ToGrid( pathRegions[i], 0 );
				vp->debug_path = TRUE;
			}
		}
	}
}


void WorldMap::ShowAdjacentRegions( float _x, float _y )
{
	int x = (int)_x;
	int y = (int)_y;
	ClearDebugDrawing();

	Grid* r = GridOrigin( x, y );
	if ( r->IsPassable() ) {
		r->debug_origin = TRUE;
		MP_VECTOR< micropather::StateCost > adj;
		AdjacentCost( ToState( x, y ), &adj );
		for( unsigned i=0; i<adj.size(); ++i ) {
			Grid* n = ToGrid( adj[i].state, 0 );
			GLASSERT( n->debug_adjacent == 0 );
			n->debug_adjacent  = 1;
		}
	}
}


void WorldMap::DrawZones()
{
	CompositingShader debug( GPUState::BLEND_NORMAL );
	debug.SetColor( 1, 1, 1, 0.5f );
	CompositingShader debugOrigin( GPUState::BLEND_NORMAL );
	debugOrigin.SetColor( 1, 0, 0, 0.5f );
	CompositingShader debugAdjacent( GPUState::BLEND_NORMAL );
	debugAdjacent.SetColor( 1, 1, 0, 0.5f );
	CompositingShader debugPath( GPUState::BLEND_NORMAL );
	debugPath.SetColor( 0.5f, 0.5f, 1, 0.5f );

	for( int j=0; j<height; ++j ) {
		for( int i=0; i<width; ++i ) {
			CalcZone( i, j );

			static const float offset = 0.1f;
			
			if ( IsRegionOrigin( i, j ) ) {
				const Grid& g = grid[INDEX(i,j)];
				if ( g.IsPassable() ) {

					Vector3F p0 = { (float)i+offset, 0.01f, (float)j+offset };
					Vector3F p1 = { (float)(i+g.size)-offset, 0.01f, (float)(j+g.size)-offset };

					if ( g.debug_origin ) {
						debugOrigin.DrawQuad( 0, p0, p1, false );
					}
					else if ( g.debug_adjacent ) {
						debugAdjacent.DrawQuad( 0, p0, p1, false );
					}
					else if ( g.debug_path ) {
						debugPath.DrawQuad( 0, p0, p1, false );
					}
					else {
						debug.DrawQuad( 0, p0, p1, false );
					}
				}
			}
		}
	}
}


void WorldMap::Submit( GPUState* shader, bool emissiveOnly )
{
	for( int i=0; i<LOWER_TYPES; ++i ) {
		if ( emissiveOnly && !texture[i]->Emissive() )
			continue;
		if ( vertex[i].Size() > 0 ) {
			GPUStream stream( vertex[i][0] );
			shader->Draw( stream, texture[i], vertex[i].Mem(), index[i].Size(), index[i].Mem() );
			}
	}
}


void WorldMap::Draw3D(  const grinliz::Color3F& colorMult, GPUState::StencilMode mode )
{

	// Real code to draw the map:
	FlatShader shader;
	shader.SetColor( colorMult.r, colorMult.g, colorMult.b );
	shader.SetStencilMode( mode );
	Submit( &shader, false );

	if ( debugRegionOverlay ) {
		if ( mode == GPUState::STENCIL_CLEAR ) {
			// Debugging pathing zones:
			DrawZones();
		}
	}
	if ( debugPathVector.Size() > 0 ) {
		FlatShader debug;
		debug.SetColor( 1, 0, 0, 1 );
		for( int i=0; i<debugPathVector.Size()-1; ++i ) {
			Vector3F tail = { debugPathVector[i].x, 0.2f, debugPathVector[i].y };
			Vector3F head = { debugPathVector[i+1].x, 0.2f, debugPathVector[i+1].y };
			debug.DrawArrow( tail, head, false );
		}
	}
	{
		// Debugging coordinate system:
		Vector3F origin = { 0, 0.1f, 0 };
		Vector3F xaxis = { 5, 0, 0 };
		Vector3F zaxis = { 0, 0.1f, 5 };

		FlatShader debug;
		debug.SetColor( 1, 0, 0, 1 );
		debug.DrawArrow( origin, xaxis, false );
		debug.SetColor( 0, 0, 1, 1 );
		debug.DrawArrow( origin, zaxis, false );
	}
}

