#include "worldmap.h"

#include "../xegame/xegamelimits.h"

#include "../grinliz/glutil.h"
#include "../grinliz/glgeometry.h"

#include "../engine/texture.h"
#include "../engine/ufoutil.h"

using namespace grinliz;

WorldMap::WorldMap( int width, int height ) : Map( width, height )
{
	GLASSERT( width % ZONE_SIZE == 0 );
	GLASSERT( height % ZONE_SIZE == 0 );

	GLASSERT( sizeof(Grid) == 4 );
	grid = 0;
	zoneInit = 0;
	pather = 0;
	Init( width, height );

	texture[0] = TextureManager::Instance()->GetTexture( "map_water" );
	texture[1] = TextureManager::Instance()->GetTexture( "map_land" );
	pather = new micropather::MicroPather( this );

	debugRegionOverlay = false;
}


WorldMap::~WorldMap()
{
	delete [] grid;
	delete [] zoneInit;
	delete pather;
}


void WorldMap::Init( int w, int h )
{
	delete [] grid;
	delete [] zoneInit;
	if ( pather ) {
		pather->Reset();
	}	
	this->width = w;
	this->height = h;
	grid = new Grid[width*height];
	zoneInit = new U8[width*height/ZONE_SIZE2];
	memset( grid, 0, width*height*sizeof(Grid) );
	memset( zoneInit, 0, sizeof(*zoneInit)*width*height/ZONE_SIZE2 );
}


void WorldMap::InitCircle()
{
	memset( grid, 0, width*height*sizeof(Grid) );
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
				grid[i].isLand = TRUE;
			}
		}
	}
	Tessellate();
}


void WorldMap::InitPNG( FILE* fp, grinliz::CDynArray<grinliz::Vector2I>* wayPoints )
{
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
			zoneInit[ZDEX( x, y )] = 0;
			GLOUTPUT(( "Block (%d,%d) index=%d zone=%d set\n", x, y, i, ZDEX(x,y) ));
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
			zoneInit[ZDEX( x, y )] = 0;
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
	static const Vector2I delta[9] = { {0,0}, {-1,-1}, {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0} };
	static const float EPSILON = 0.0001f;

	for( int i=0; i<9; ++i ) {
		Vector2I block = blockPos + delta[i];
		if (    b.Contains(block) 
			 && !grid[INDEX(block.x,block.y)].IsPassable() ) 
		{
			// Will find the smallest overlap, and apply.
			Vector2F c = { (float)block.x+0.5f, (float)block.y+0.5f };	// block center.
			Vector2F p = pos - c;										// translate pos to origin
			Vector2F n = p; n.Normalize();

			if ( p.x > fabsf(p.y) && (p.x-rad < 0.5f) ) {				// east quadrant
				//force->Set( 0.5f - (p.x-rad) + EPSILON, 0 );
				float dx = 0.5f - (p.x-rad) + EPSILON;
				GLASSERT( dx > 0 );
				*force = n * (dx/fabsf(n.x));
				return FORCE_APPLIED;
			}
			if ( -p.x > fabsf(p.y) && (p.x+rad > -0.5f) ) {				// west quadrant
				//force->Set( -0.5f- (p.x+rad) - EPSILON, 0 );
				float dx = 0.5f + (p.x+rad) + EPSILON;
				*force = n * (dx/fabsf(n.x));
				GLASSERT( dx > 0 );
				return FORCE_APPLIED;
			}
			if ( p.y > fabsf(p.x) && (p.y-rad < 0.5f) ) {				// north quadrant
				//force->Set( 0, 0.5f - (p.y-rad) + EPSILON );
				float dy = 0.5f - (p.y-rad) + EPSILON;
				*force = n * (dy/fabsf(n.y));
				GLASSERT( dy > 0 );
				return FORCE_APPLIED;
			}
			if ( -p.y > fabsf(p.x) && (p.y+rad > -0.5f) ) {				// south quadrant
				//force->Set( 0, -0.5f - (p.y+rad) - EPSILON );
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

	// Can't think of a case where it's possible to overlap more than 2.
	// But if this asserts in some strange case, can up the # checks to 3.
	for( int i=0; i<2; ++i ) {
		BlockResult result = CalcBlockEffect( *outPos, radius, &force );
		if ( result == STUCK )
			return STUCK;
		if ( result == FORCE_APPLIED )
			*outPos += force;	
		if ( result == NO_EFFECT )
			break;
	}
//	GLASSERT( CalcBlockEffect( *outPos, radius, &force ) == NO_EFFECT );
	return ( *outPos == inPos ) ? NO_EFFECT : FORCE_APPLIED;
}


bool WorldMap::RectPassable( int x0, int y0, int x1, int y1 )
{
	for( int y=y0; y<=y1; ++y ) {
		for( int x=x0; x<=x1; ++x ) {
			if ( !grid[INDEX(x,y)].IsPassable() || grid[INDEX(x,y)].isPathInit )
				return false;
		}
	}
	return true;
}


void WorldMap::CalcZone( int zx, int zy )
{
	zx = zx & (~(ZONE_SIZE-1));
	zy = zy & (~(ZONE_SIZE-1));

	if ( zoneInit[ZDEX(zx,zy)] == 0 ) {
		GLOUTPUT(( "CalcZone (%d,%d) %d\n", zx, zy, ZDEX(zx,zy) ));
		zoneInit[ZDEX(zx,zy)] = 1;

		for( int y=zy; y<zy+ZONE_SIZE; ++y ) {
			for( int x=zx; x<zx+ZONE_SIZE; ++x ) {
				grid[INDEX(x,y)].isPathInit = FALSE;
			}
		}

		int xEnd = zx + ZONE_SIZE;
		int yEnd = zy + ZONE_SIZE;
		static const int NUM_PASS = 2;	// 1 or 2

		// 178 dc using grow-from-origin
		// 190 dc using 2-pass, but looks much better. (grow square always better,
		// the question is the 2-pass approach)
		for( int pass=0; pass<NUM_PASS; ++pass ) {
			for( int y=zy; y<yEnd; ++y ) {
				for( int x=zx; x<xEnd; ++x ) {

					int index = INDEX(x,y);
					if ( !grid[index].IsPassable() )
						continue;
					if ( grid[index].isPathInit )
						continue;

					bool maybeNegX = true,
						 maybePosX = true,
						 maybeNegY = true,
						 maybePosY = true;
					int x0, y0, x1, y1;
					x0 = x1 = x;
					y0 = y1 = y;

					while( maybeNegX || maybePosX || maybeNegY || maybePosY ) {
						if ( maybeNegX ) {
							if ( x0-1 >= zx && RectPassable( x0-1, y0, x0-1, y1 ) )
								--x0;
							else
								maybeNegX = false;
						}
						if ( maybeNegY ) {
							if ( y0-1 >= zy && RectPassable( x0, y0-1, x1, y0-1 ) )
								--y0;
							else
								maybeNegY = false;
						}
						if ( maybePosX ) {
							if ( (x1+1 < zx+ZONE_SIZE) && RectPassable( x1+1, y0, x1+1, y1 ) )
								++x1;
							else
								maybePosX = false;
						}
						if ( maybePosY ) {
							if ( (y1+1 < zy+ZONE_SIZE) && RectPassable( x0, y1+1, x1, y1+1 ) )
								++y1;
							else
								maybePosY = false;
						}
					}
					int dx = x1-x0+1;
					int dy = y1-y0+1;
					int area = dx*dy;
					// If the 2nd pass of a 2 pass algorithm,
					// filter out things that aren't square enough.
					if ( pass == NUM_PASS-2 ) {
						if ( (dx > dy*2) || (dy > dx*2) )
							continue;
					}

					for( int j=y0; j<=y1; ++j ) {
						for( int i=x0; i<=x1; ++i ) {
							GLASSERT( !grid[INDEX(i,j)].isPathInit );
							grid[ INDEX( i, j ) ].SetPathOrigin( i-x0, j-y0, dx, dy );
						}
					}
				}
			}
		}
	}
}


int WorldMap::NumRegions() const
{
	int nSubZone = 0;
	for( int i=0; i<width*height; ++i ) {
		if ( grid[i].IsRegionOrigin() )
			++nSubZone;
	}
	return nSubZone;
}


float WorldMap::LeastCostEstimate( void* stateStart, void* stateEnd )
{
	int startX, startY, endX, endY;

	Grid gStart = ToGrid( stateStart, &startX, &startY );
	Grid gEnd =   ToGrid( stateEnd, &endX, &endY );

	Vector2F start = { (float)startX + (float)gStart.sizeX * 0.5f,
	                   (float)startY + (float)gStart.sizeY * 0.5f };
	Vector2F end   = { (float)endX + (float)gEnd.sizeX * 0.5f,
					   (float)endY + (float)gEnd.sizeY * 0.5f };
	return (end-start).Length();
}


void WorldMap::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	int startX, startY;
	Grid gStart = ToGrid( state, &startX, &startY );
	Vector2F start = { (float)startX + (float)gStart.sizeX * 0.5f,
	                   (float)startY + (float)gStart.sizeY * 0.5f };

	Rectangle2I bounds = this->Bounds();
	Vector2I currentSubZone = { -1, -1 };

	Vector2I adj[ZONE_SIZE*4+4];
	int num=0;

	int dx = (int)gStart.sizeX;
	int dy = (int)gStart.sizeY;

	for( int x=startX; x < startX+dx; ++x )		{ adj[num++].Set( x, startY-1 ); }
	if (    bounds.Contains( startX+dx, startY-1 )
		 && grid[INDEX(startX+dx-1, startY-1)].IsPassable()
		 && grid[INDEX(startX+dx,   startY-1)].IsPassable()
		 && grid[INDEX(startX+dx,   startY)].IsPassable() )
		{ adj[num++].Set( startX+dx, startY-1 ); }
	for( int y=startY; y < startY+dy; ++y )		{ adj[num++].Set( startX+gStart.sizeX, y ); }
	if (    bounds.Contains( startX+dx, startY+dy )
		 && grid[INDEX(startX+dx,   startY+dy-1)].IsPassable()
		 && grid[INDEX(startX+dx,   startY+dy)].IsPassable()
		 && grid[INDEX(startX+dx-1, startY+dy)].IsPassable() )
		{ adj[num++].Set( startX+dx, startY+dy ); }
	for( int x=startX+(int)gStart.sizeX-1; x >= startX; --x )	{ adj[num++].Set( x, startY+gStart.sizeY ); }
	if (    bounds.Contains( startX-1, startY+dy )
		 && grid[INDEX(startX,   startY+dy)].IsPassable()
		 && grid[INDEX(startX-1, startY+dy)].IsPassable()
		 && grid[INDEX(startX-1, startY+dy-1)].IsPassable() )
		{ adj[num++].Set( startX-1, startY+dy ); }
	for( int y=startY+(int)gStart.sizeY-1; y >= startY; --y )	{ adj[num++].Set( startX-1, y ); }
	if (    bounds.Contains( startX-1, startY-1 )
		 && grid[INDEX(startX-1, startY)].IsPassable()
		 && grid[INDEX(startX-1, startY-1)].IsPassable()
		 && grid[INDEX(startX,   startY-1)].IsPassable() )
		{ adj[num++].Set( startX-1, startY-1 ); }

	for( int i=0; i<num; ++i ) {
		int x = adj[i].x;
		int y = adj[i].y;

		if ( bounds.Contains( x, y ) ) {
			Vector2I subV = GetRegion( x, y );
			if ( subV.x >= 0 && subV != currentSubZone ) {
				Grid g = grid[INDEX(subV.x,subV.y)];
				Vector2F end = { (float)subV.x + (float)g.sizeX * 0.5f,
								 (float)subV.y + (float)g.sizeY * 0.5f };
				float cost = (end-start).Length();
				
				micropather::StateCost sc = { ToState( subV.x, subV.y ), cost };
				adjacent->push_back( sc );
				currentSubZone = subV;
			}
		}
	}
}


void WorldMap::PrintStateInfo( void* state )
{
	int startX, startY;
	Grid gStart = ToGrid( state, &startX, &startY );
	GLOUTPUT(( "(%d,%d) ", startX, startY ));	
}


int WorldMap::RegionSolve( const grinliz::Vector2I& subZoneStart, const grinliz::Vector2I& subZoneEnd )
{
	void* start = ToState( subZoneStart.x, subZoneStart.y );
	void* end   = ToState( subZoneEnd.x, subZoneEnd.y );

	float totalCost = 0;
	int result = pather->Solve( start, end, &pathRegions, &totalCost );
	return result;
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

	// Just do special checks up front:
	if ( dx == 0 || dy == 0 ) {
		int x0 = (int)Min( p0.x, p1.x );
		int y0 = (int)Min( p0.y, p1.y );
		int x1 = (int)Max( p0.x, p1.x );
		int y1 = (int)Max( p0.y, p1.y );
		return RectPassable( x0, y0, x1, y1 );
	}

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
							bool debugging )
{
	pathCache.Clear();
	*len = 0;
	bool result = CalcPath( start, end, &pathCache, debugging );
	if ( result ) {
		for( int i=0; i<pathCache.Size() && i < maxPath; ++i ) {
			path[i] = pathCache[i];
		}
		*len = Min( maxPath, pathCache.Size() );
	}
	return result;
}


bool WorldMap::CalcPath(	const grinliz::Vector2F& start, 
							const grinliz::Vector2F& end, 
							CDynArray<grinliz::Vector2F> *path,
							bool debugging )
{
	debugPathVector.Clear();
	path->Clear();
	bool okay = false;

	Vector2I regionStart = GetRegion( (int)start.x, (int)start.y );
	Vector2I regionEnd   = GetRegion( (int)end.x,   (int)end.y );

	// Check for bad pathing coordinates.
	if ( regionStart.x < 0 || regionEnd.x < 0 )
		return false;

	if ( regionStart == regionEnd ) {
		okay = true;
		path->Push( start );
		path->Push( end );
	}

	// Try a straight line ray cast
	if ( !okay ) {
		okay = GridPath( start, end );
		if ( okay ) {
			GLOUTPUT(( "Ray succeeded.\n" ));
			path->Push( start );
			path->Push( end );
		}
	}

	// Use the region solver.
	if ( !okay ) {
		int result = RegionSolve( regionStart, regionEnd );
		if ( result == micropather::MicroPather::SOLVED ) {
			GLOUTPUT(( "Region succeeded len=%d.\n", pathRegions.size() ));
			Vector2F from = start;
			path->Push( start );
			okay = true;

			// Walk each of the regions, and connect them with vectors.
			for( unsigned i=0; i<pathRegions.size()-1; ++i ) {
				Vector2I vA, vB;
				Grid gA = ToGrid( pathRegions[i], &vA.x, &vA.y );
				Grid gB = ToGrid( pathRegions[i+1], &vB.x, &vB.y );

				Rectangle2F bA, bB;
				gA.CalcBounds( (float)vA.x, (float)vA.y, &bA );
				gB.CalcBounds( (float)vB.x, (float)vB.y, &bB );
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
		grid[i].debug_adjacent = 0;
		grid[i].debug_origin = 0;
		grid[i].debug_path = 0;
	}
	debugPathVector.Clear();
}


void WorldMap::ShowRegionPath( float x0, float y0, float x1, float y1 )
{
	ClearDebugDrawing();
	
	Vector2I start = GetRegion( (int)x0, (int)y0 );
	Vector2I end   = GetRegion( (int)x1, (int)y1 );
	
	if ( start.x >= 0 && end.x >= 0 ) {
		int result = RegionSolve( start, end );
		if ( result == micropather::MicroPather::SOLVED ) {
			for( unsigned i=0; i<pathRegions.size(); ++i ) {
				void* vp = pathRegions[i];
				int x, y;
				ToGrid( vp, &x, &y );
				grid[INDEX(x,y)].debug_path = TRUE;
			}
		}
	}
}


void WorldMap::ShowAdjacentRegions( float _x, float _y )
{
	int x = (int)_x;
	int y = (int)_y;
	ClearDebugDrawing();

	if ( grid[INDEX(x,y)].IsPassable() ) {
		Vector2I sub = GetRegion( x, y );
		grid[INDEX(sub.x,sub.y)].debug_origin = TRUE;

		MP_VECTOR< micropather::StateCost > adjacent;
		AdjacentCost( ToState( sub.x, sub.y ), &adjacent );
		for( unsigned i=0; i<adjacent.size(); ++i ) {
			Vector2I a;
			ToGrid( adjacent[i].state, &a.x, &a.y );
			grid[INDEX(a.x,a.y)].debug_adjacent = TRUE;
		}
	}
}


void WorldMap::DrawZones()
{
	CompositingShader debug( GPUShader::BLEND_NORMAL );
	debug.SetColor( 1, 1, 1, 0.5f );
	CompositingShader debugOrigin( GPUShader::BLEND_NORMAL );
	debugOrigin.SetColor( 1, 0, 0, 0.5f );
	CompositingShader debugAdjacent( GPUShader::BLEND_NORMAL );
	debugAdjacent.SetColor( 1, 1, 0, 0.5f );
	CompositingShader debugPath( GPUShader::BLEND_NORMAL );
	debugPath.SetColor( 0.5f, 0.5f, 1, 0.5f );

	for( int j=0; j<height; ++j ) {
		for( int i=0; i<width; ++i ) {
			if ( !zoneInit[ZDEX(i,j)] ) {
				CalcZone( i, j );
			}

			static const float offset = 0.2f;
			Grid g = grid[INDEX(i,j)];
			
			if ( g.IsRegionOrigin() ) {
				int xSize = g.sizeX;
				int ySize = g.sizeY;

				Vector3F p0 = { (float)i+offset, 0.1f, (float)j+offset };
				Vector3F p1 = { (float)(i+xSize)-offset, 0.1f, (float)(j+ySize)-offset };

				if ( g.debug_origin ) {
					debugOrigin.DrawQuad( p0, p1, false );
				}
				else if ( g.debug_adjacent ) {
					debugAdjacent.DrawQuad( p0, p1, false );
				}
				else if ( g.debug_path ) {
					debugPath.DrawQuad( p0, p1, false );
				}
				else {
					debug.DrawQuad( p0, p1, false );
				}
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

	// Real code to draw the map:
	FlatShader shader;
	shader.SetColor( colorMult.r, colorMult.g, colorMult.b );
	shader.SetStencilMode( mode );
	for( int i=0; i<LOWER_TYPES; ++i ) {
		shader.SetStream( stream, vertex[i].Mem(), index[i].Size(), index[i].Mem() );
		shader.SetTexture0( texture[i] );
		shader.Draw();
	}

	if ( debugRegionOverlay ) {
		if ( mode == GPUShader::STENCIL_CLEAR ) {
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

