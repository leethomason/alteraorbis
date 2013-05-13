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
#include "../xegame/chitevent.h"
#include "../xegame/chitbag.h"

#include "../grinliz/glutil.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glperformance.h"
#include "../shared/lodepng.h"
#include "../xarchive/glstreamer.h"

#include "../engine/engine.h"
#include "../engine/texture.h"
#include "../engine/ufoutil.h"
#include "../engine/surface.h"

#include "worldinfo.h"
#include "gameitem.h"
#include "../script/worldgen.h"

using namespace grinliz;
using namespace micropather;
using namespace tinyxml2;

static const int POOL_HEIGHT = 2;

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
	ShaderManager::Instance()->AddDeviceLossHandler( this );

	grid = 0;
	engine = 0;
	currentPather = 0;
	worldInfo = 0;
	Init( width, height );
	slowTick = SLOW_TICK;
	iMapGridUse = 0;

	texture[0] = TextureManager::Instance()->GetTexture( "map_water" );
	texture[1] = TextureManager::Instance()->GetTexture( "map_grid" );
	texture[2] = TextureManager::Instance()->GetTexture( "map_port" );
	texture[3] = TextureManager::Instance()->GetTexture( "map_core" );
	texture[4] = TextureManager::Instance()->GetTexture( "map_land" );

	debugRegionOverlay = false;
}


WorldMap::~WorldMap()
{
	GLASSERT( engine == 0 );
	ShaderManager::Instance()->RemoveDeviceLossHandler( this );

	DeleteAllRegions();
	delete [] grid;
	delete worldInfo;
}


void WorldMap::DeviceLoss()
{
	FreeVBOs();
	Tessellate();
}

void WorldMap::FreeVBOs()
{
	for( int i=0; i<WorldGrid::NUM_LAYERS; ++i ) {
		vertexVBO[i].Destroy();
		indexVBO[i].Destroy();
	}
}


const SectorData* WorldMap::GetSectorData() const
{
	return worldInfo->SectorDataMem();
}


const SectorData& WorldMap::GetSector( int x, int y ) const
{
	Vector2I sector = { x / SECTOR_SIZE, y/SECTOR_SIZE };
	return worldInfo->GetSector( sector );
}


const SectorData& WorldMap::GetSector( const Vector2I& sector ) const
{
	return worldInfo->GetSector( sector );
}


void WorldMap::AttachEngine( Engine* e, IMapGridUse* imap ) 
{
	GLASSERT( (e==0) || (e!=0 && engine==0) );

	if ( !e ) {
		for( int j=0; j<height; ++j ) {
			for( int i=0; i<width; ++i ) {
				if ( grid[INDEX(i,j)].IsLand() ) {
					SetRock( i, j, 0, 0, false );
				}
			}
		}
	}
	GLASSERT( e==0 || voxels.Empty() );
	engine = e;
	iMapGridUse = imap;
}


void WorldMap::SavePNG( const char* path )
{
	Color4U8* pixels = new Color4U8[width*height];

	for( int i=0; i<width*height; ++i ) {
		pixels[i] = grid[i].ToColor();
	}
	lodepng_encode32_file( path, (const unsigned char*)pixels, width, height );

	delete [] pixels;
}


void WorldMap::Save( const char* pathToDAT )
{
	// Debug or laptap, about 4.5MClock
	// smaller window size: 3.8MClock
	// btype == 0 about the same.
	// None of this matters; may need to add an ultra-simple fast encoder.

	FILE* fp = fopen( pathToDAT, "wb" );
	GLASSERT( fp );
	if ( fp ) {

		StreamWriter writer(fp);

		XarcOpen( &writer, "Map" );
		XARC_SER( &writer, width );
		XARC_SER( &writer, height );
		
		worldInfo->Serialize( &writer );
		XarcClose( &writer );

		// Tack on the grid so that the dat file can still be inspected.
		fwrite( grid, sizeof(WorldGrid), width*height, fp );
		fclose( fp );
	}
}


void WorldMap::Load( const char* pathToDAT )
{
	FILE* fp = fopen( pathToDAT, "rb" );
	GLASSERT( fp );
	if ( fp ) {
		StreamReader reader( fp );
		
		XarcOpen( &reader, "Map" );

		XARC_SER( &reader, width );
		XARC_SER( &reader, height );
		Init( width, height );

		worldInfo->Serialize( &reader );
		XarcClose( &reader );

		fread( grid, sizeof(WorldGrid), width*height, fp );
		fclose( fp );
		
		Tessellate();
		usingSectors = true;
		// Set up the rocks.
		for( int j=0; j<height; ++j ) {
			for( int i=0; i<width; ++i ) {
				int index = INDEX( i, j );
				const WorldGrid& wg = grid[index];
				SetRock( i, j, -2, 0, grid[index].Magma() );
			}
		}
	}
}


void WorldMap::PatherCacheHitMiss( const grinliz::Vector2I& sector, micropather::CacheData* data )
{
	const SectorData& sd = worldInfo->GetSector( sector );
	if ( sd.pather ) {
		sd.pather->GetCacheData( data );
	}
}


int WorldMap::CalcNumRegions()
{
	int count = 0;
	if ( grid ) {
		// Delete all the regions. Be careful to only
		// delete from the origin location.
		for( int j=0; j<height; ++j ) {	
			for( int i=0; i<width; ++i ) {
				if ( IsZoneOrigin( i, j )) {
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
				if ( IsPassable(i,j) && IsZoneOrigin(i, j)) {
					const WorldGrid& gs = grid[INDEX(i,j)];
					GLOUTPUT(( "Region %d,%d size=%d", i, j, gs.ZoneSize() ));
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
	// Reset the voxels
	if ( engine ) {
		Engine* savedEngine = engine;
		IMapGridUse* savedIMap = iMapGridUse;
		AttachEngine( 0, 0 );
		AttachEngine( savedEngine, savedIMap );
	}

	waterInit.ClearAll();
	DeleteAllRegions();
	delete [] grid;
	this->width = w;
	this->height = h;
	grid = new WorldGrid[width*height];
	memset( grid, 0, width*height*sizeof(WorldGrid) );
	
	delete worldInfo;
	worldInfo = new WorldInfo( grid, width, height );
}


void WorldMap::InitCircle()
{
	memset( grid, 0, width*height*sizeof(WorldGrid) );

	const int R = Min( width, height )/2;
	const int R2 = R * R;
	const int cx = width/2;
	const int cy = height/2;

	for( int y=0; y<height; ++y ) {
		for( int x=0; x<width; ++x ) {
			int r2 = (x-cx)*(x-cx) + (y-cy)*(y-cy);
			if ( r2 < R2 ) {
				int i = INDEX( x, y );
				grid[i].SetLand();
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
				grid[i].SetLand();
				blocks->Push( p );
			}
			else if ( c.r == c.g && c.g == c.b ) {
				grid[i].SetLand();
				color = c.r;
			}
			else if ( c == BLUE ) {
				grid[i].SetWater();
			}
			else if ( c == RED ) {
				grid[i].SetLand();
				wayPoints->Push( p );
			}
			else if ( c == GREEN ) {
				grid[i].SetLand();
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


void WorldMap::MapInit( const U8* land )
{
	GLASSERT( grid );
	for( int i=0; i<width*height; ++i ) {
		int h = *(land + i);
		if ( h >= WorldGen::WATER && h <= WorldGen::LAND3 ) {
			grid[i].SetLandAndRock( h );
		}
		else if ( h == WorldGen::GRID ) {
			grid[i].SetGrid();
		}
		else if ( h == WorldGen::PORT ) {
			grid[i].SetPort();
		}
		else if ( h == WorldGen::CORE ) {
			grid[i].SetCore();
		}
		else {
			GLASSERT( 0 );
		}
	}
}


bool WorldMap::Similar( const grinliz::Rectangle2I& r, int layer, const BitArray<MAX_MAP_SIZE, MAX_MAP_SIZE, 1 >& setmap )
{
	if ( !setmap.IsRectEmpty( r ) ) {
		return false;
	}

	for( int y=r.min.y; y<=r.max.y; ++y ) {
		for( int x=r.min.x; x<=r.max.x; ++x ) {
			const WorldGrid& wg = grid[INDEX(x,y)];
			if ( wg.Layer() != layer )
				return false;
		}
	}
	return true;
}


void WorldMap::PushQuad( int layer, int x, int y, int w, int h, CDynArray<PTVertex>* vertex, CDynArray<U16>* index ) 
{
	U16* pi = index->PushArr( 6 );
	int base = vertex->Size();

	pi[0] = base;
	pi[1] = base+3;
	pi[2] = base+2;
	pi[3] = base;
	pi[4] = base+1;
	pi[5] = base+3;

	PTVertex* pv = vertex->PushArr( 4 );
	pv[0].pos.Set( (float)x, 0, (float)y );
	pv[0].tex.Set( 0, 0 );

	pv[1].pos.Set( (float)x, 0, (float)(y+h) );
	pv[1].tex.Set( 0, (float)h );

	pv[2].pos.Set( (float)(x+w), 0, (float)(y) );
	pv[2].tex.Set( (float)w, 0 );

	pv[3].pos.Set( (float)(x+w), 0, (float)(y+h) );
	pv[3].tex.Set( (float)w, (float)h );
}


// Wow: Tessallate: land:87720,131580 water:73236,109854
// Filtering:
// Tessallate:      land:76504,114756 water:60944, 91416
// Growing v/h:
// Tessallate:		land:46448, 69672 water:39296, 58944
// NCORES 100->60
// Tessallate:		land:41056, 61584 water:34464, 51696
// Still 2/3 the limit. But seems reasonably stable.
//
void WorldMap::Tessellate()
{
	CDynArray<PTVertex>*	vertex[WorldGrid::NUM_LAYERS];
	CDynArray<U16>*			index[WorldGrid::NUM_LAYERS];
	for( int i=0; i<WorldGrid::NUM_LAYERS; ++i ) {
		vertex[i] = new CDynArray<PTVertex>();
		index[i]  = new CDynArray<U16>();
	}

	BitArray<MAX_MAP_SIZE, MAX_MAP_SIZE, 1 >* setmap = new BitArray<MAX_MAP_SIZE, MAX_MAP_SIZE, 1>();
	Rectangle2I r, r0, r1;

	for( int j=0; j<height; ++j ) {
		for( int i=0; i<width; ++i ) {
			if ( !setmap->IsSet( i, j ) ) {
				int x = i;	int y = j; int w = 1; int h = 1;
				int layer = grid[INDEX(i,j)].Layer();

				// Try to grow square.
				while( x+w < width && y+h < height ) {
					r0.Set( x+w, y, x+w, y+h );
					r1.Set( x, y+h, x+w, y+h );

					if ( Similar( r0, layer, *setmap ) && Similar( r1, layer, *setmap ) ) {
						++w;
						++h;
					}
					else {
						break;
					}
				}
				// Grow to the right.
				while( x+w < width ) {
					r0.Set( x+w, y, x+w, y+h-1 );
					if ( Similar( r0, layer, *setmap ) ) {
						++w;
					}
					else {
						break;
					}
				}
				// Grow down.
				while( y+h < height ) {
					r0.Set( x, y+h, x+w-1, y+h );
					if ( Similar( r0, layer, *setmap ) ) {
						++h;
					}
					else {
						break;
					}
				}
				r.Set( x, y, x+w-1, y+h-1 );
				setmap->SetRect( r );
				PushQuad( layer, x, y, w, h, vertex[layer], index[layer] ); 
			}
		}
	}
	delete setmap;

	FreeVBOs();

	GLOUTPUT(( "Tessallate: land:%d,%d water:%d,%d\n", vertex[WorldGrid::LAND]->Size(),  index[WorldGrid::LAND]->Size(),
													   vertex[WorldGrid::WATER]->Size(), index[WorldGrid::WATER]->Size() ));
	for( int i=0; i<WorldGrid::NUM_LAYERS; ++i ) {
		vertexVBO[i] = GPUVertexBuffer::Create( vertex[i]->Mem(), sizeof(PTVertex), vertex[i]->Size() );
		indexVBO[i]  = GPUIndexBuffer::Create( index[i]->Mem(), index[i]->Size() );
		nIndex[i] = index[i]->Size();
		delete vertex[i];
		delete index[i];
	}
}


Vector2I WorldMap::FindEmbark()
{
	Random random;

	const SectorData* s = 0;
	while( !s ) {
		s = worldInfo->SectorDataMem() + random.Rand(NUM_SECTORS*NUM_SECTORS);
		if ( !s->HasCore() ) {
			s = 0;
		}
	}

	Vector2I v = s->core;
	v.x += 2;
	v.y += 2;
	return v;
}



void WorldMap::ProcessWater( ChitBag* cb )
{
	//QuickProfile qp( "WorldMap::ProcessWater" );
	static const Vector2I next[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
	int zoneWidth  = width / ZONE_SIZE;
	int zoneHeight = height / ZONE_SIZE;

	waterStack.Clear();
	poolGrids.Clear();

	// Walk each zone, look for something that needs update.
	for( int zy=0; zy<zoneHeight; ++zy ) {
		for( int zx=0; zx<zoneWidth; ++zx ) {
			if ( !waterInit.IsSet( zx, zy )) {

				waterInit.Set( zx, zy, 0, true );
				//zoneInfo[zy*DZONE+zx].pools = 0;
				const int baseX = zx*ZONE_SIZE;
				const int baseY = zy*ZONE_SIZE;
				
				CArray<U8, ZONE_SIZE2> color;
				color.PushArr( ZONE_SIZE2 );
				memset( color.Mem(), 0, ZONE_SIZE2 );

				Rectangle2I zbounds;
				zbounds.Set( baseX, baseY, baseX+ZONE_SIZE-1, baseY+ZONE_SIZE-1 );

				// Clear the waterfalls for this zone.
				for( int i=0; i<waterfalls.Size(); ++i ) {
					if ( zbounds.Contains( waterfalls[i] )) {
						waterfalls.SwapRemove( i );
						--i;
					}
				}

				// Scan for possible pools. This is a color fill. Look
				// for colors that don't touch the border, or too much
				// water.
				for( int dy=1; dy<ZONE_SIZE-1; ++dy ) {
					for( int dx=1; dx<ZONE_SIZE-1; ++dx ) {
						const int x = baseX + dx;
						const int y = baseY + dy;

						// Do a fill of everything land rockHeight < POOL_HEIGHT
						// Note boundary conditions
						// Determine pool.
						
						int currentColor = 1;
						int index = INDEX( x,y );

						if (    grid[index].IsLand() 
							 && grid[index].RockHeight() < POOL_HEIGHT	// need space for the pool
							 && color[dy*ZONE_SIZE+dx] == 0 )			// don't revisit
						{
							// Try a fill!
							Vector2I start = { x, y };
							waterStack.Push( start );
							color[(start.y-baseY)*ZONE_SIZE + start.x-baseX] = currentColor;
							poolGrids.Clear();

							int border = 0;
							int water = 0;

							while ( !waterStack.Empty() ) {

								Vector2I top = waterStack.Pop();
								poolGrids.Push( top );
								GLASSERT( color[(top.y-baseY)*ZONE_SIZE + top.x-baseX] == currentColor );

								if (    top.x == zbounds.min.x || top.x == zbounds.max.x
									 || top.y == zbounds.min.y || top.y == zbounds.max.y )
								{
									++border;
								}

								for( int i=0; i<4; ++i ) {
									Vector2I v = top + next[i];

									if ( !zbounds.Contains( v ))
										continue;

									int idx = INDEX(v);

									if (    grid[idx].IsLand()
										 && grid[idx].RockHeight() < POOL_HEIGHT
										 && color[(v.y-baseY)*ZONE_SIZE + v.x-baseX] == 0 )
									{
										color[(v.y-baseY)*ZONE_SIZE + v.x-baseX] = currentColor;
										waterStack.Push( v );
									}
									else if ( grid[idx].IsWater() ) {
										++water;
									}
								}
							}

							int waterMax = poolGrids.Size() / 10;
							if ( poolGrids.Size() >= 10 && border == 0 && water <= waterMax ) {
								GLOUTPUT(( "pool found. zone=%d,%d area=%d waterFall=%d\n", zx, zy, poolGrids.Size(), water ));
								
								Vector2F posF = { (float)poolGrids[0].x+0.5f, (float)poolGrids[0].y+0.5f };
								NewsEvent poolNews( NewsEvent::UNICORN, posF, StringPool::Intern( "water", true ));
								cb->AddNews( poolNews );

								for( int i=0; i<poolGrids.Size(); ++i ) {
									int idx = INDEX( poolGrids[i] );
									SetRock( poolGrids[i].x, poolGrids[i].y, grid[idx].RockHeight(), POOL_HEIGHT, grid[idx].Magma() );	

									for( int k=0; k<4; ++k ) {
										Vector2I v = poolGrids[i] + next[k];
										GLASSERT( zbounds.Contains( v ));
										if ( grid[INDEX(v)].IsWater() ) {
											waterfalls.Push( poolGrids[i] );

											posF.Set( (float)poolGrids[i].x+0.5f, (float)poolGrids[i].y+0.5f );
											NewsEvent we( NewsEvent::PEGASUS, posF, StringPool::Intern( "waterfall" ));
											cb->AddNews( we );
											
											break;
										}
									}
								}
								//zoneInfo[zy*DZONE+zx].pools += 1;
							}
							++currentColor;
							poolGrids.Clear();
						}
					}
				}
			}
		}
	}
}



void WorldMap::EmitWaterfalls( U32 delta )
{
	static const Vector2I next[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
	for( int i=0; i<waterfalls.Size(); ++i ) {
		const Vector2I& wf = waterfalls[i];

		ParticleDef pdWater = engine->particleSystem->GetPD( "fallingwater" );
		ParticleDef pdMist  = engine->particleSystem->GetPD( "mist" );

		for( int j=0; j<4; ++j ) {
			Vector2I v = wf + next[j];
			if ( grid[INDEX(v)].IsWater() ) {
				Vector3F v3 = { (float)wf.x + 0.5f, (float)POOL_HEIGHT, (float)wf.y + 0.5f };
				Vector3F half = { (float)next[j].x*0.5f, 0.0f, (float)next[j].y*0.5f };
				v3 = v3 + half*1.2f;

				Rectangle3F r3;
				half.Set( half.z, half.y, half.x );
				r3.FromPair( v3-half, v3+half );

				static const Vector3F DOWN = { 0, -1, 0 };
				engine->particleSystem->EmitPD( pdWater, r3, DOWN, engine->camera.EyeDir3(), delta ); 

				r3.min.y = r3.max.y = (float)POOL_HEIGHT - 0.2f;
				engine->particleSystem->EmitPD( pdMist, r3, V3F_UP, engine->camera.EyeDir3(), delta ); 
				r3.min.y = r3.max.y = 0.0f;
				engine->particleSystem->EmitPD( pdMist, r3, V3F_UP, engine->camera.EyeDir3(), delta ); 
			}
		}
	}
}

void WorldMap::DoTick( U32 delta, ChitBag* chitBag )
{
	ProcessWater( chitBag );
	EmitWaterfalls( delta );

	slowTick -= (int)(delta);

	// Send fire damage events, if needed.
	if ( slowTick <= 0 ) {
		slowTick = SLOW_TICK;
		for( int i=0; i<magmaGrids.Size(); ++i ) {
			Vector2F origin = { (float)magmaGrids[i].x+0.5f, (float)magmaGrids[i].y + 0.5f };
			ChitEvent event = ChitEvent::EffectEvent( origin, EFFECT_RADIUS, GameItem::EFFECT_FIRE, EFFECT_ACCRUED_MAX );
			chitBag->QueueEvent( event );
		}
	}

	// Do particles every time.
	ParticleDef pdEmber = engine->particleSystem->GetPD( "embers" );
	ParticleDef pdSmoke = engine->particleSystem->GetPD( "smoke" );
	for( int i=0; i<magmaGrids.Size(); ++i ) {
		Rectangle3F r;
		r.min.Set( (float)magmaGrids[i].x, 0, (float)magmaGrids[i].y );
		r.max = r.min;
		r.max.x += 1.0f; r.max.z += 1.0f;
		
		int index = INDEX(magmaGrids[i]);
		if ( grid[index].IsWater() || grid[index].PoolHeight() ) {
			r.min.y =  r.max.y = (float)grid[index].PoolHeight();
			engine->particleSystem->EmitPD( pdSmoke, r, V3F_UP, engine->camera.EyeDir3(), delta );
		}
		else {
			r.min.y = r.max.y = (float)grid[index].RockHeight();
			engine->particleSystem->EmitPD( pdSmoke, r, V3F_UP, engine->camera.EyeDir3(), delta );
		}
	}
}


void WorldMap::SetRock( int x, int y, int h, int pool, bool magma )
{
	// what is there now? h=[1,3]
	// does it need to be changed removed?
	// do blocks need to be set/removed?

	Vector2I vec = { x, y };
	int index = INDEX(x,y);

	if ( !grid[index].IsLand() ) {
		return;
	}

	bool wasPassable = grid[index].IsPassable();
	int  hWas = grid[index].RockHeight();
	int  pWas = grid[index].PoolHeight();	
	bool mWas = grid[index].Magma();

	if ( h == -1 ) {
		h = grid[index].NominalRockHeight();
		if ( iMapGridUse ) {
			GLASSERT( iMapGridUse->MapGridUse( x, y ) == 0 );
		}
	}
	else if ( h == -2 ) {
		hWas = 0;
		pWas = 0;
		mWas = false;
		h     = grid[index].RockHeight();
		pool  = grid[index].PoolHeight();
		magma = grid[index].Magma();
		if ( iMapGridUse ) {
			GLASSERT( iMapGridUse->MapGridUse( x, y ) == 0 );
		}
	}
	else {
		if ( iMapGridUse ) {
			if ( iMapGridUse->MapGridUse( x, y )) {
				GLASSERT( hWas == 0 && pWas == 0 );
				return;
			}
		}
	}

	CStr<12> name; 
	bool modelWas = false;

	GridResName( grid[index].IsLand(), hWas, pWas, mWas, &name );
	if ( name.Length() ) {
		modelWas = true;
	}


#ifdef DEBUG
	if ( pool > 0 )
		GLASSERT( pool > h );
	if ( engine ) {
		if ( modelWas ) {
			Model* m = 0;
			voxels.Query( vec, &m );
			GridResName( grid[index].IsLand(), hWas, pWas, mWas, &name );
			GLASSERT( m && m->GetResource()->header.name == name.c_str() );
		}
		else {
			GLASSERT( voxels.Query( vec, 0 ) == false );
		}
	}
#endif

	if ( h != hWas || pool != pWas || magma != mWas ) {

		if ( engine ) {
			if ( modelWas ) {
				Model* m = voxels.Remove( vec );
				engine->FreeModel( m );
			}
			GridResName( grid[index].IsLand(), h, pool, magma, &name );
			if ( name.Length() ) {
				const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( name.c_str() );
				Model* m = engine->AllocModel( res );
				m->SetPos( (float)vec.x+0.5f, 0.0f, (float)vec.y+0.5f );
				GLASSERT( m );
				voxels.Add( vec, m );
			}
		}
		grid[index].SetRockHeight( h );
		grid[index].SetPoolHeight( pool );
		grid[index].SetMagma( magma );
		if ( wasPassable != grid[index].IsPassable() ) {
			ResetPather( x, y );
		}

		if ( h != hWas ) {
			waterInit.Clear( vec.x >> ZONE_SHIFT, vec.y >> ZONE_SHIFT );
		}
		if ( magma != mWas ) {
			if ( mWas ) {
				// Magma going away.
				int i = magmaGrids.Find( vec );
				GLASSERT( i >= 0 );
				if ( i >= 0 ) {
					magmaGrids.Remove( i );
				}
			}
			else {
				// Magma adding.
				magmaGrids.Push( vec );
			}
		}
	}
}


void WorldMap::ResetPather( int x, int y )
{
	Vector2I sector = { x/SECTOR_SIZE, y/SECTOR_SIZE };
	micropather::MicroPather* pather = worldInfo->GetSector( sector ).pather;
	if ( pather ) {
		pather->Reset();
	}
	zoneInit.Clear( x>>ZONE_SHIFT, y>>ZONE_SHIFT);
}


bool WorldMap::IsPassable( int x, int y ) const
{
	int index = INDEX(x,y);
	if ( grid[index].IsPassable() ) {
		int flags = 0;
		if ( iMapGridUse ) {
			flags = iMapGridUse->MapGridUse( x, y );
		}
		return ( flags & GRID_BLOCKED ) == 0;
	}
	return false;
}


Vector2I WorldMap::FindPassable( int x, int y )
{
	int c = 0;

	while ( true ) {
		int x0 = Max( 0, x-c );
		int x1 = Min( width-1, x+c );
		int y0 = Max( 0, y-c );
		int y1 = Min( height-1, y+c );

		for( int j=y0; j<=y1; ++j ) {
			for( int i=x0; i<=x1; ++i ) {
				if ( j==y0 || j==y1 || i==x0 || i==x1 ) {
					if ( IsPassable(i,j) ) {
						Vector2I v = { i, j };
						return v;
					}
				}
			}
		}
		++c;
	}
	GLASSERT( 0 );	// The world is full?
	Vector2I vz = { 0, 0 };
	return vz;
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
			&& !IsPassable(block.x, block.y) ) 
		{
			// Will find the smallest overlap, and apply.
			Vector2F c = { (float)block.x+0.5f, (float)block.y+0.5f };	// block center.
			Vector2F p = pos - c;										// translate pos to origin
			Vector2F n = p; 
			if ( n.LengthSquared() )
				n.Normalize();
			else
				n.Set( 1, 0 );

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


void WorldMap::CalcZone( int mapX, int mapY )
{
	struct SZ {
		U8 x;
		U8 y;
		U8 size;
	};

	int zx = mapX & (~(ZONE_SIZE-1));
	int zy = mapY & (~(ZONE_SIZE-1));

	if ( !zoneInit.IsSet( zx>>ZONE_SHIFT, zy>>ZONE_SHIFT) ) {
		int mask[ZONE_SIZE];
		memset( mask, 0, sizeof(int)*ZONE_SIZE );
		CArray< SZ, ZONE_SIZE*ZONE_SIZE > stack;

		//GLOUTPUT(( "CalcZone (%d,%d) %d\n", zx, zy, ZDEX(zx,zy) ));
		zoneInit.Set( zx>>ZONE_SHIFT, zy>>ZONE_SHIFT);

		// Build up a bit pattern to analyze.
		for( int y=0; y<ZONE_SIZE; ++y ) {
			for( int x=0; x<ZONE_SIZE; ++x ) {
				if ( IsPassable(zx+x,zy+y) ) {
					mask[y] |= 1 << x;
				}
				else {
					grid[INDEX(zx+x, zy+y)].SetZoneSize(0);
				}
			}
		}

		SZ start = { 0, 0, ZONE_SIZE };
		stack.Push( start );

		while( !stack.Empty() ) {
			const SZ sz = stack.Pop();
			
			// So we can quickly check for everything set,
			// create a mask to iterate over the bitf
			int m = (1<<sz.size)-1;
			m <<= sz.x;

			int j=sz.y;
			for( ; j<sz.y+sz.size; ++j ) {
				if ( (mask[j] & m) != m )
					break;
			}

			if ( j == sz.y + sz.size ) {
				// Match.
				for( int y=sz.y; y<sz.y+sz.size; ++y ) {
					for( int x=sz.x; x<sz.x+sz.size; ++x ) {
						grid[INDEX(zx+x, zy+y)].SetZoneSize(sz.size);
					}
				}
			}
			else if ( sz.size > 1 ) {
				SZ sz00 = { sz.x, sz.y, sz.size/2 };
				SZ sz10 = { sz.x+sz.size/2, sz.y, sz.size/2 };
				SZ sz01 = { sz.x, sz.y+sz.size/2, sz.size/2 };
				SZ sz11 = { sz.x+sz.size/2, sz.y+sz.size/2, sz.size/2 };
				stack.Push( sz00 );
				stack.Push( sz10 );
				stack.Push( sz01 );
				stack.Push( sz11 );
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

	Vector2F start = ZoneCenter( startI.x, startI.y );
	Vector2F end   = ZoneCenter( endI.x, endI.y );
	return (end-start).Length();
}


// micropather
void WorldMap::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent )
{
	Vector2I start;
	const WorldGrid* startGrid = ToGrid( state, &start );

	// Flush out the neighbors.
	for( int j=-1; j<=1; ++j ) {
		for( int i=-1; i<=1; ++i ) {
			int x = start.x + i*ZONE_SIZE;
			int y = start.y + j*ZONE_SIZE;
			if ( x >= 0 && x < width && y >= 0 && y < height ) {
				CalcZone( x, y ); 
			}
		}
	}

	GLASSERT( IsPassable( start.x, start.y ));
	Vector2F startC = ZoneCenter( start.x, start.y );
	
	Rectangle2I bounds = this->Bounds();
	Vector2I currentZone = { -1, -1 };
	CArray< Vector2I, ZONE_SIZE*4+4 > adj;

	int size = startGrid->ZoneSize();
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
				if ( IsPassable(v.x,v.y) ) 
				{
					Vector2I origin = ZoneOrigin( v.x, v.y );
					GLASSERT( ToState( v.x, v.y ) != state );
					GLASSERT( IsPassable( origin.x, origin.y) );
					adj.Push( v );
				}
			}
		}
		// Check the corner.
		bool pass = true;
		for( int j=0; j<3; ++j ) {
			Vector2I delta = { cornerDir[i].x * filter[j].x, cornerDir[i].y * filter[j].y };
			v = corner[i] + delta;
			if ( bounds.Contains( v ) && IsPassable( v.x, v.y ) ) {
				// all is well.
			}
			else {
				pass = false;
				break;
			}
		}
		if ( pass ) {
			GLASSERT( ToState( corner[i].x, corner[i].y ) != state );
			GLASSERT( IsPassable( corner[i].x, corner[i].y) );
			adj.Push( corner[i] );
		}
	}

	const WorldGrid* current = 0;
	const WorldGrid* first = 0;
	for( int i=0; i<adj.Size(); ++i ) {
		int x = adj[i].x;
		int y = adj[i].y;
		GLASSERT( bounds.Contains( x, y ));

		CalcZone( x, y );
		const WorldGrid* g = ZoneOriginG( x, y );
		GLASSERT( IsPassable( x,y ) );

		// The corners wrap around:
		if ( g == current || g == first)
		{
			continue;
		}
		current = g;
		if ( !first )
			first = g;

		Vector2F otherC = ZoneCenter( x, y );
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
	WorldGrid* g = ToGrid( state, &vec );
	int size = g->ZoneSize();
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

    for (; n > 0; --n) {
		CalcZone( x,y );

        if ( !IsPassable(x,y) )
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


SectorPort WorldMap::NearestPort( const Vector2F& pos )
{
	Vector2I secPos = { (int)pos.x / SECTOR_SIZE, (int)pos.y / SECTOR_SIZE };
	const SectorData& sd = worldInfo->GetSector( secPos );

	int   bestPort = 0;
	float bestCost = FLT_MAX;

	for( int i=0; i<4; ++i ) {
		int port = (1<<i);
		if ( sd.ports & port ) {
			Vector2I desti = sd.GetPortLoc( port ).Center();
			Vector2F dest = { (float)desti.x, (float)desti.y };
			float cost = FLT_MAX;

			if ( CalcPath( pos, dest, 0, &cost, false ) ) {
				if ( cost < bestCost ) {
					bestCost = cost;
					bestPort = port;
				}
			}
		}
	}
	SectorPort result;
	if ( bestPort ) {
		result.port = bestPort;
		result.sector = secPos;
	}
	return result;
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


micropather::MicroPather* WorldMap::PushPather( const Vector2I& sector )
{
	GLASSERT( currentPather == 0 );
	GLASSERT( sector.x >= 0 && sector.x < NUM_SECTORS );
	GLASSERT( sector.y >= 0 && sector.y < NUM_SECTORS );
	SectorData* sd = worldInfo->SectorDataMemMutable() + sector.y*NUM_SECTORS+sector.x;
	if ( !sd->pather ) {
		int area = sd->area ? sd->area : 1000;
		sd->pather = new micropather::MicroPather( this, area, 7, true );
	}
	currentPather = sd->pather;
	GLASSERT( currentPather );
	return currentPather;
}


bool WorldMap::CalcPath(	const grinliz::Vector2F& start, 
							const grinliz::Vector2F& end, 
							CDynArray<grinliz::Vector2F> *path,
							float *totalCost,
							bool debugging )
{
	debugPathVector.Clear();
	if ( path ) { 
		path->Clear();
	}
	bool okay = false;

	Vector2I starti = { (int)start.x, (int)start.y };
	Vector2I endi   = { (int)end.x,   (int)end.y };
	Vector2I sector = { starti.x/SECTOR_SIZE, starti.y/SECTOR_SIZE };

	// Flush out regions that aren't valid.
	// Don't do this. Use the AdjacentCost for this.
	//for( int j=0; j<height; j+= ZONE_SIZE ) {
	//	for( int i=0; i<width; i+=ZONE_SIZE ) {
	//		CalcZone( i, j );
	//	}
	//}

	// But do flush the current region.
	CalcZone( starti.x, starti.x );
	CalcZone( endi.x,   endi.y );

	WorldGrid* regionStart = grid + INDEX( starti.x, starti.y );
	WorldGrid* regionEnd   = grid + INDEX( endi.x, endi.y );

	if ( !IsPassable( starti.x, starti.y ) || !IsPassable( endi.x, endi.y ) ) {
		return false;
	}

	// Regions are convex. If in the same region, it is passable.
	if ( regionStart == regionEnd ) {
		okay = true;
		if ( path ) {
			path->Push( start );
			path->Push( end );
		}
		*totalCost = (end-start).Length();
	}

	// Try a straight line ray cast
	if ( !okay ) {
		okay = GridPath( start, end );
		if ( okay ) {
			if ( path ) { 
				path->Push( start );
				path->Push( end );
			}
			*totalCost = (end-start).Length();
		}
	}

	// Use the region solver.
	if ( !okay ) {
		micropather::MicroPather* pather = PushPather( sector );

		int result = pather->Solve( ToState( starti.x, starti.y ), ToState( endi.x, endi.y ), 
								    &pathRegions, totalCost );
		if ( result == micropather::MicroPather::SOLVED ) {
			//GLOUTPUT(( "Region succeeded len=%d.\n", pathRegions.size() ));
			Vector2F from = start;
			if ( path ) { 
				path->Push( start );
			}
			okay = true;
			//Vector2F pos = start;

			// Walk each of the regions, and connect them with vectors.
			for( unsigned i=0; i<pathRegions.size()-1; ++i ) {
				Vector2I originA, originB;
				WorldGrid* rA = ToGrid( pathRegions[i], &originA );
				WorldGrid* rB = ToGrid( pathRegions[i+1], &originB );

				Rectangle2F bA = ZoneBounds( originA.x, originA.y );
				Rectangle2F bB = ZoneBounds( originB.x, originB.y );					
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
				if ( path ) {
					path->Push( v );
				}
				from = v;
			}
			if ( path ) {
				path->Push( end );
			}
		}
		PopPather();
	}

	if ( okay ) {
		if ( debugging && path ) {
			for( int i=0; i<path->Size(); ++i )
				debugPathVector.Push( (*path)[i] );
		}
	}
	else {
		if ( path ) {
			path->Clear();
		}
	}
	return okay;
}


void WorldMap::ClearDebugDrawing()
{
	for( int i=0; i<width*height; ++i ) {
		grid[i].SetDebugAdjacent( false );
		grid[i].SetDebugOrigin( false );
		grid[i].SetDebugPath( false );
	}
	debugPathVector.Clear();
}


void WorldMap::ShowRegionPath( float x0, float y0, float x1, float y1 )
{
	ClearDebugDrawing();

	void* start = ToState( (int)x0, (int)y0 );
	void* end   = ToState( (int)x1, (int)y1 );
	Vector2I sector = { (int)x0/SECTOR_SIZE, (int)y0/SECTOR_SIZE };
	
	if ( start && end ) {
		float cost=0;
		micropather::MicroPather* pather = PushPather( sector );
		int result = pather->Solve( start, end, &pathRegions, &cost );

		if ( result == micropather::MicroPather::SOLVED ) {
			for( unsigned i=0; i<pathRegions.size(); ++i ) {
				WorldGrid* vp = ToGrid( pathRegions[i], 0 );
				vp->SetDebugPath( true );
			}
		}
		PopPather();
	}
}


void WorldMap::ShowAdjacentRegions( float fx, float fy )
{
	int x = (int)fx;
	int y = (int)fy;
	ClearDebugDrawing();

	if ( IsPassable( x, y ) ) {
		WorldGrid* r = ZoneOriginG( x, y );
		r->SetDebugOrigin( true );

		MP_VECTOR< micropather::StateCost > adj;
		AdjacentCost( ToState( x, y ), &adj );
		for( unsigned i=0; i<adj.size(); ++i ) {
			WorldGrid* n = ToGrid( adj[i].state, 0 );
			GLASSERT( n->DebugAdjacent() == false );
			n->SetDebugAdjacent( true );
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
			
			if ( IsZoneOrigin( i, j ) ) {
				if ( IsPassable( i, j ) ) {

					const WorldGrid& gs = grid[INDEX(i,j)];
					Vector3F p0 = { (float)i+offset, 0.01f, (float)j+offset };
					Vector3F p1 = { (float)(i+gs.ZoneSize())-offset, 0.01f, (float)(j+gs.ZoneSize())-offset };

					if ( gs.DebugOrigin() ) {
						debugOrigin.DrawQuad( 0, p0, p1, false );
					}
					else if ( gs.DebugAdjacent() ) {
						debugAdjacent.DrawQuad( 0, p0, p1, false );
					}
					else if ( gs.DebugPath() ) {
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
	for( int i=0; i<WorldGrid::NUM_LAYERS; ++i ) {
		if ( emissiveOnly && !texture[i]->Emissive() )
			continue;
		if ( vertexVBO[i].IsValid() && indexVBO[i].IsValid() ) {
			PTVertex pt;
			GPUStream stream( pt );
			shader->Draw( stream, texture[i], vertexVBO[i], nIndex[i], indexVBO[i] );
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

