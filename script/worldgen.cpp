#include "worldgen.h"

#include <string.h>

#include "../grinliz/glnoise.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glbitarray.h"

#include "../shared/lodepng.h"
#include "../game/gamelimits.h"
#include "../engine/ufoutil.h"

#include "../game/worldinfo.h"
#include "../game/worldgrid.h"
#include "../game/lumosmath.h"

using namespace grinliz;

static const float BASE0 = 8.f;
static const float BASE1 = 64.f;
static const float EDGE = 0.1f;
static const float OCTAVE = 0.18f;

SectorData::~SectorData()
{
}

void SectorData::Serialize(XStream* xs)
{
	XarcOpen(xs, "SectorData");
	XARC_SER(xs, sector);
	XARC_SER(xs, ports);
	XARC_SER(xs, core);
	XARC_SER(xs, area);
	XARC_SER(xs, name);
	XARC_SER(xs, defaultSpawn);
	XarcClose(xs);
}


WorldGen::WorldGen()
{
	land = new U8[MAX_MAP_SIZE_2];
	memset(land, 0, MAX_MAP_SIZE_2*sizeof(*land));
	color = new U16[MAX_MAP_SIZE_2];
	path = new U16[MAX_MAP_SIZE_2];
	memset(path, 0, sizeof(*path)*MAX_MAP_SIZE_2);
	flixels = 0;
	noise0 = 0;
	noise1 = 0;
	features = 0;
}


WorldGen::~WorldGen()
{
	delete [] land;
	delete [] color;
	delete [] flixels;
	delete [] path;
	delete noise0;
	delete noise1;
	if ( features ) free( features );
}


void WorldGen::LoadFeatures( const char* path )
{
	lodepng_decode32_file( (unsigned char**)(&features), (unsigned int*) &featuresSize.x, (unsigned int*) &featuresSize.y, path );
}

void WorldGen::StartLandAndWater( U32 seed0, U32 seed1 )
{
	flixels = new float[MAX_MAP_SIZE*MAX_MAP_SIZE];

	noise0 = new PerlinNoise( seed0 );
	noise1 = new PerlinNoise( seed1 );
}


void WorldGen::DoLandAndWater(int j)
{
	GLASSERT(j >= 0 && j < MAX_MAP_SIZE);
	for (int i = 0; i < MAX_MAP_SIZE; ++i) {
		float nx = (float)i / (float)MAX_MAP_SIZE;
		float ny = (float)j / (float)MAX_MAP_SIZE;

		// Noise layer.
		float n0 = noise0->Noise2(BASE0*nx, BASE0*ny);
		float n1 = noise1->Noise2(BASE1*nx, BASE1*ny);

		float n = n0 + n1*OCTAVE;
		n = PerlinNoise::Normalize(n);

		// Water at the edges.
		float dEdge = Min(nx, 1.0f - nx);
		if (dEdge < EDGE)		n = Lerp(0.f, n, dEdge / EDGE);
		dEdge = Min(ny, 1.0f - ny);
		if (dEdge < EDGE)		n = Lerp(0.f, n, dEdge / EDGE);

		flixels[j*MAX_MAP_SIZE + i] = n;
	}
}


bool WorldGen::EndLandAndWater( float fractionLand )
{
	float cutoff = fractionLand;
	int target = (int)((float)(MAX_MAP_SIZE*MAX_MAP_SIZE)*fractionLand);
	float high = 1.0f;
	int highCount = 0;
	float low = 0.0f;
	int lowCount = MAX_MAP_SIZE*MAX_MAP_SIZE;
	int iteration=0;
	static const int MAX_ITERATION = 10;
	float maxh = 1.0f;

	while( iteration<MAX_ITERATION ) {
		int count = CountFlixelsAboveCutoff( flixels, cutoff, &maxh );
		if ( count > target ) {
			low = cutoff;
			lowCount = count;
		}
		else {
			high = cutoff;
			highCount = count;
		}
		if ( abs(count - target) < (target/10) ) {
			break;
		}
		//cutoff = (high+low)*0.5f;
		cutoff = low + (high-low)*(float)(lowCount - target) / (float)(lowCount - highCount);
		++iteration;
	}

	for( int j=0; j<MAX_MAP_SIZE; ++j ) {
		for( int i=0; i<MAX_MAP_SIZE; ++i ) {
			if ( flixels[j*MAX_MAP_SIZE+i] > cutoff ) {
				float flix = (flixels[j*MAX_MAP_SIZE+i] - cutoff) / (maxh-cutoff);
				GLASSERT( flix >= 0 && flix <= 1 );
				int h = LAND0 + (int)(flix*3.5f);
				GLASSERT( h > 0 && h <= 4 );
				land[j*MAX_MAP_SIZE+i] = h;
			}
			else {
				land[j*MAX_MAP_SIZE+i] = 0;
			}
		}
	}

	delete [] flixels;	flixels = 0;
	delete noise0;		noise0 = 0;
	delete noise1;		noise1 = 0;
	return iteration < MAX_ITERATION;
}


void WorldGen::WriteMarker()
{
	static const int S = 8;
	for ( int y=0; y<S; ++y ) {
		for( int x=0; x<S; ++x ) {
			if ( y==0 || y==(S-1) || x==0 || x==(S-1) ) {
				land[y*MAX_MAP_SIZE+x] = 0;
			}
			else {
				land[y*MAX_MAP_SIZE+x] = 1;
			}
		}
	}
}


int WorldGen::CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh )
{
	int count = 0;
	*maxh = 0.0f;
	for( int j=0; j<MAX_MAP_SIZE; ++j ) {
		for( int i=0; i<MAX_MAP_SIZE; ++i ) {
			float f = flixels[j*MAX_MAP_SIZE+i];
			if ( f > cutoff ) {
				++count;
				if ( f > *maxh ) *maxh = f;
			}
		}
	}
	return count;
}


int WorldGen::Color( const Rectangle2I& bounds, const Vector2I& origin )
{
	// Rember that (playable) land is fully connected. This
	// is guaranteed in GenerateTerrain(), which
	// simply deletes all the land not connected 
	// to the core and ports.

	// The AI can use this information to get
	// to a core or port from any location is
	// the map, and in fact, we encode this into
	// the map.

	// This algorithm is a flood fill from 
	// the destination to every point on the
	// map. The correct direction to the 
	// destination is the the direction of
	// "one less" in the resulting gradient.

	for( int y=bounds.min.y; y <= bounds.max.y; ++y ) {
		for( int x=bounds.min.x; x <= bounds.max.x; ++x ) {
			color[INDEX(x,y)] = 0;
		}
	}
	int c = 1;
	CDynArray<Vector2I> stack;

	GLASSERT( land[origin.y*MAX_MAP_SIZE+origin.x] );

	stack.Push( origin );
	color[INDEX(origin.x, origin.y)] = 1;
	int area = 1;

	while( !stack.Empty() ) {
		Vector2I v = stack.PopFront();
		c = color[INDEX(v)] + 1;
		GLASSERT( c > 1 );

		static const Vector2I d[4] = { {1,0}, {0,1}, {-1,0}, {0,-1} };

		for( int k=0; k<4; ++k ) {
			Vector2I n = v + d[k];
			if ( bounds.Contains(n) && land[INDEX(n)] && color[INDEX(n)] == 0 ) {
				color[INDEX(n)] = c;
				++area;
				stack.Push( n );
			}
		}
	}
	return area;
}


int WorldGen::PortsColored( const SectorData* s )
{
	int portsColored = 0;
	for( int i=0; i<4; ++i ) {
		int port = 1<<i;
		if ( s->ports & port ) {
			Rectangle2I r = s->GetPortLoc( port );
			if ( color[INDEX(r.min)] ) {
				portsColored |= port;
			}
		}
	}
	return portsColored;
}


void WorldGen::RemoveUncoloredLand( SectorData* s )
{
	Rectangle2I bounds = s->InnerBounds();

	for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
		for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
			if ( color[y*MAX_MAP_SIZE+x] == 0 ) {
				land[y*MAX_MAP_SIZE+x] = 0;
			}
		}
	}
}


void WorldGen::DepositLand( SectorData* s, U32 seed, int n )
{
	Rectangle2I bounds = s->InnerBounds();
	CDynArray<Vector2I> stack;

	// Inset one more, so that checks can be in all directions.
	for( int y=bounds.min.y+1; y<=bounds.max.y-1; ++y ) {
		for( int x=bounds.min.x+1; x<=bounds.max.x-1; ++x ) {
			if (    land[y*MAX_MAP_SIZE+x]		// is land
			     && (    land[y*MAX_MAP_SIZE+x+1] == 0
					  || land[y*MAX_MAP_SIZE+x-1] == 0
					  || land[(y+1)*MAX_MAP_SIZE+x] == 0
					  || land[(y-1)*MAX_MAP_SIZE+x] == 0 ))
			{
				Vector2I v = { x, y };
				stack.Push( v );
			}
		}
	}

	// now grow
	Random random( seed );
	Vector2I dir[4] = { {-1,0}, {1,0}, {0,-1}, {0,1} };

	ShuffleArray( stack.Mem(), stack.Size(), &random );

	while( !stack.Empty() && n ) {
		GLASSERT( stack.Size() < SECTOR_SIZE*SECTOR_SIZE );
		int d = Min( 4, stack.Size()-1 );
		int index = d > 1 ? (stack.Size()-1-random.Rand(d)) : (stack.Size()-1);
		Vector2I origin = stack[index];
		GLASSERT( land[origin.y*MAX_MAP_SIZE+origin.x] );

		bool deposit = false;
		for( int i=0; i<4; ++i ) {
			Vector2I c = origin + dir[i];
			if ( bounds.Contains(c) && land[c.y*MAX_MAP_SIZE+c.x] == 0 ) {
				land[c.y*MAX_MAP_SIZE+c.x] = LAND0;
				deposit = true;
				stack.Push( c );
				--n;
				break;
			}
		}
		if ( !deposit ) {
			stack.SwapRemove( index );
		}
		ShuffleArray( dir, 4, &random );
	}
}


void WorldGen::Draw( const Rectangle2I& r, int isLand )
{
	GLASSERT( r.min.x >= 0 && r.max.x < MAX_MAP_SIZE );
	GLASSERT( r.min.y >= 0 && r.max.y < MAX_MAP_SIZE );
	for( int y=r.min.y; y<=r.max.y; ++y ) {
		for( int x=r.min.x; x<=r.max.x; ++x ) {
			land[y*MAX_MAP_SIZE+x] = isLand;
		}
	}
}


void WorldGen::CutRoads( U32 seed, SectorData* sectorData )
{
	// Each sector is represented by one pather,
	// so they can't connect. However, this also
	// means that each sector should be fully connected,
	// or not connected at all. (Future water/pirate
	// expansion.)
	
	//First pass: split up the world.
	for( int pass=0; pass<2; ++pass ) {
		for( int j=0; j<NUM_SECTORS-1; ++j ) {
			Rectangle2I r;
			r.Set( 0,	   (j+1)*SECTOR_SIZE-1, 
				   MAX_MAP_SIZE-1, (j+1)*SECTOR_SIZE );
			if ( pass == 1 ) {
				Swap( &r.min.x, &r.min.y );
				Swap( &r.max.x, &r.max.y );
			}

			Draw( r, WorldGrid::WATER );
		}
	}

	// Set which sectors are on the road. When roads are
	// everything which can be a road is a road.
	BitArray< NUM_SECTORS, NUM_SECTORS, 1 > sectors;
	Random random( seed );
	Vector2I center = { NUM_SECTORS/2, NUM_SECTORS/2 };
	int rad = NUM_SECTORS/2;

	// Set the basic:
	Rectangle2I r;
	r.Set(1, 1, NUM_SECTORS - 2, NUM_SECTORS - 2);
	sectors.SetRect(r);

	// Clear corners:
	sectors.Clear(1, 1);
	sectors.Clear(1, NUM_SECTORS - 2);
	sectors.Clear(NUM_SECTORS - 2, 1);
	sectors.Clear(NUM_SECTORS - 2, NUM_SECTORS - 2);
	int nCores = (NUM_SECTORS - 2) * (NUM_SECTORS - 2) - 4;

	// 16x16 -> 120 domains
	// 8x8 -> 30 domains
	static const int NUM_CORES = 120 * NUM_SECTORS_2 / (16 * 16);

	while (nCores > NUM_CORES) {
		int y = random.Rand(NUM_SECTORS);
		if (random.Bit()) {
			for (int x = 0; x < center.x - 2; ++x) {
				if (sectors.IsSet(x, y)) {
					sectors.Clear(x, y);
					nCores--;
					break;
				}
			}
		}
		else {
			for (int x = NUM_SECTORS-1; x > center.x + 1; --x) {
				if (sectors.IsSet(x, y)) {
					sectors.Clear(x, y);
					nCores--;
					break;
				}
			}
		}
	}

	// Now fill in roads.
	for( int j=1; j<NUM_SECTORS-1; ++j ) {
		for( int i=1; i<NUM_SECTORS-1; ++i ) {
			if ( sectors.IsSet( i, j )) {
				Rectangle2I r;

				int x0 = i*SECTOR_SIZE;
				int x1 = (i+1)*SECTOR_SIZE-1;
				int y0 = j*SECTOR_SIZE;
				int y1 = (j+1)*SECTOR_SIZE-1;

				bool left   = sectors.IsSet(i-1,j) ? true : false;
				bool right  = sectors.IsSet(i+1,j) ? true : false;
				bool up     = sectors.IsSet(i,j-1) ? true : false;
				bool down   = sectors.IsSet(i,j+1) ? true : false;

				// Make sure we are connected in.
				if ( i < rad ) {
					if ( j < rad ) {
						down = true;
						right = true;
					}
					else { 
						up = true;
						right = true;
					}
				}
				else {
					if ( j < rad ) {
						down = true;
						left = true;
					}
					else {	
						up = true;
						left = true;
					}
				}

				if ( left ) {
					r.Set( x0-1, y0-1, x0, y1+1 );
					Draw( r, GRID );
					sectorData[j*NUM_SECTORS+i].ports |= SectorData::NEG_X;
				}
				if ( right ) {
					r.Set( x1, y0+1, x1+1, y1+1 );
					Draw( r, GRID );
					sectorData[j*NUM_SECTORS+i].ports |= SectorData::POS_X;
				}
				if ( up ) {
					r.Set( x0-1, y0-1, x1+1, y0 );
					Draw( r, GRID );
					sectorData[j*NUM_SECTORS+i].ports |= SectorData::NEG_Y;
				}
				if ( down ) {
					r.Set( x0-1, y1, x1+1, y1+1 );
					Draw( r, GRID );
					sectorData[j*NUM_SECTORS+i].ports |= SectorData::POS_Y;
				}
			}
		}
	}
}


int WorldGen::CalcSectorArea( int sx, int sy )
{
	int area = 0;
	for( int j=sy*SECTOR_SIZE+1;
		 j < (sy+1)*SECTOR_SIZE-1;
		 ++j )
	{
		for( int i=sx*SECTOR_SIZE+1;
			 i < (sx+1)*SECTOR_SIZE-1;
			 ++i )
		{
			if ( land[j*MAX_MAP_SIZE+i] ) {
				++area;
			}
		}
	}
	return area;
}


Rectangle2I SectorData::Bounds() const { 
	return SectorBounds(sector);
}


Rectangle2I SectorData::InnerBounds() const { 
	return InnerSectorBounds(sector);
}


int SectorData::RandomPort(Random* random) const
{
	int arr[4] = { 1, 2, 4, 8 };
	random->ShuffleArray(arr, 4);
	for (int i = 0; i < 4; ++i) {
		if (arr[i] & ports) {
			return arr[i];
		}
	}
	GLASSERT(0);
	return 0;
}


int SectorData::NearestPort( const grinliz::Vector2I& pos ) const
{
	int bestDist = INT_MAX;
	int bestPort = 0;
	for( int i=0; i<4; ++i ) {
		int port = (1<<i);
		if ( ports & port ) {
			Vector2I center = GetPortLoc( port ).Center();
			int dist = abs(center.x-pos.x) + abs(center.y-pos.y);
			if ( dist < bestDist ) {
				bestDist = dist;
				bestPort = port;
			}
		}
	}
	return bestPort;
}


Rectangle2I SectorData::GetPortLoc( int port ) const
{
	Rectangle2I r;
	r.Set(0, 0, 0, 0);
//	static const int LONG  = 4;
	static const int SHORT = 2;
	static const int HALF  = SECTOR_SIZE/2;

	int x = Bounds().min.x;
	int y = Bounds().min.y;

	if ( port == NEG_X ) {
		r.Set( x+1,			y+HALF-SHORT, 
			   x+SHORT,		y+HALF+SHORT-1 ); 
	}
	else if ( port == POS_X ) {
		r.Set( x+SECTOR_SIZE-SHORT-1,	y+HALF-SHORT, 
			   x+SECTOR_SIZE-2,			y+HALF+SHORT-1 ); 
	}
	else if ( port == NEG_Y ) {
		r.Set( x+HALF-SHORT,        y+1, 
			   x+HALF+SHORT-1,		y+SHORT ); 
	}
	else if ( port == POS_Y ) {
		r.Set( x+HALF-SHORT,        y+SECTOR_SIZE-SHORT-1, 
			   x+HALF+SHORT-1,		y+SECTOR_SIZE-2 ); 
	}
	else {
		GLASSERT( 0 );
	}
	return r;
}


Vector2F SectorData::PortPos( const grinliz::Rectangle2I portRect, U32 seed )
{
	Random random( seed );
	Vector2F dest;
	dest.x = (float)(portRect.min.x + random.Rand( portRect.Width() )) + 0.5f;
	dest.y = (float)(portRect.min.y + random.Rand( portRect.Height())) + 0.5f;
	return dest;
}



void WorldGen::AddPorts( SectorData* s )
{
	for( int i=0; i<4; ++i ) {
		int port = 1 << i;
		if ( s->ports & port ) {
			Rectangle2I r = s->GetPortLoc( port );
			Draw( r, PORT );
		}
	}
}


class SectorPtrArea
{
public:
	static bool Less( const SectorData* s0, const SectorData* s1 ) {
		return s0->area < s1->area;
	}
};

void WorldGen::ProcessSectors( U32 seed, SectorData* sectorData )
{
	Random random( seed );

	// Calc the area.
	// Sort by area.
	// Choose N and fill in for cores.

	// This impacts the look of the world,
	// complexity, run cost. Change, but
	// change with caution.
	CDynArray<SectorData*> sectors;

	for( int j=0; j<NUM_SECTORS; ++j ) {
		for( int i=0; i<NUM_SECTORS; ++i ) {
			SectorData* s = &sectorData[j*NUM_SECTORS+i];
			s->sector.Set(i, j);
			if ( s->ports ) {
				AddPorts( s );
				sectors.Push( s );
			}
			s->area = CalcSectorArea( i, j );
		}
	}

	GLOUTPUT(( "nSectors=%d\n", sectors.Size() ));
	for( int i=0; i<sectors.Size(); ++i ) {
		GenerateTerrain( random.Rand(), sectors[i] );
		CalcPath( sectors[i] );
	}
}


void WorldGen::Filter( const Rectangle2I& bounds )
{
	for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
		for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
			if (    land[y*MAX_MAP_SIZE+x] == WATER 
				 && land[(y-1)*MAX_MAP_SIZE+x] != WATER
				 && land[(y+1)*MAX_MAP_SIZE+x] != WATER
				 && land[y*MAX_MAP_SIZE+(x+1)] != WATER
				 && land[y*MAX_MAP_SIZE+(x-1)] != WATER )
			{
				land[y*MAX_MAP_SIZE+x] = LAND0;
			}
		}
	}
}


void WorldGen::GenerateTerrain( U32 seed, SectorData* s )
{
	static const int AREA = INNER_SECTOR_SIZE*INNER_SECTOR_SIZE / 2;

	Random random( seed );

	// Place core
	// Random core placement doesn't really add anything.
	//Vector2I c = {	s->x + SECTOR_SIZE/2 - 10 + random.Dice( 3, 6 ),
	//				s->y + SECTOR_SIZE/2 - 10 + random.Dice( 3, 6 )  };
	Vector2I c = s->Bounds().Center();
	s->core = c;
	Vector2I sector = s->sector;

	// Make sure there is a land patch around the core.
	// Want it to be reasonably connected.
	Rectangle2I r;
	r.max = r.min = c;
	r.Outset( 2 );
	Draw( r, LAND0 );

	land[c.y*MAX_MAP_SIZE+c.x]	= CORE;
	int  a				= Color( s->InnerBounds(), s->core );
	int portsColored	= PortsColored( s );
	int	featurePlaced	= random.Rand( 2 );

	while ( a < AREA || (portsColored != s->ports) ) {
		if ( !featurePlaced && features ) {
			
			int nFeatures = (featuresSize.x/SECTOR_SIZE)*(featuresSize.y/SECTOR_SIZE);
			int feature = random.Rand( nFeatures );

			for( int i=0; i<4; ++i ) {
				int port = 1<<i;
				if ( ( port & s->ports ) && !(portsColored & port) ) {
					if ( port == SectorData::POS_X )	  PlaceFeature( feature, sector, 0 );
					else if ( port == SectorData::POS_Y ) PlaceFeature( feature, sector, 90 );
					else if ( port == SectorData::NEG_X ) PlaceFeature( feature, sector, 180 );
					else if ( port == SectorData::NEG_Y ) PlaceFeature( feature, sector, 270 );
				}
			}
			featurePlaced = 1;
		}
		else {
			DepositLand( s, seed, Max( AREA-a, (int)INNER_SECTOR_SIZE ));
		}
		Filter( s->InnerBounds() );

		a = Color( s->InnerBounds(), s->core );
		portsColored = PortsColored( s );
	}
	RemoveUncoloredLand( s );

	s->area = a;
	int checkArea = CalcSectorArea(s->sector.x, s->sector.y);
	GLASSERT( a == checkArea );
	(void)checkArea;
}


void WorldGen::PlaceFeature( int feature, const grinliz::Vector2I& sector, int rotation )
{
	if ( !features ) return;

	Matrix2I rot;
	rot.SetRotation( rotation );

	int cx = featuresSize.x / SECTOR_SIZE;
	int fy = feature / cx;
	int fx = feature - fy*cx;

	// Mapping
	Rectangle2I map;
	map.Set( 0, 0, SECTOR_SIZE-1, SECTOR_SIZE-1 );
	map = rot * map;

	for( int j=0; j<SECTOR_SIZE; ++j ) {
		for( int i=0; i<SECTOR_SIZE; ++i ) {
			Color4U8 c = features[(fy*SECTOR_SIZE+j)*featuresSize.x + (fx*SECTOR_SIZE+i)];
			Vector2I src = { i, j };
			Vector2I dst = rot * src;
			dst.x += -map.min.x + sector.x * SECTOR_SIZE;
			dst.y += -map.min.y + sector.y * SECTOR_SIZE;

			if ( c.g() == 255 ) {
				if ( land[INDEX(dst)] == WATER ) {
					land[INDEX(dst)] = LAND0;
				}
			}
			else if ( c.b() == 255 ) {
				if ( land[INDEX(dst)] >= LAND0 && land[INDEX(dst)] <= LAND3 ) {
					land[INDEX(dst)] = WATER;
				}
			}
		}
	}
}

void WorldGen::CalcPath( const SectorData* s )
{
	Rectangle2I bounds = s->InnerBounds();
	static const Vector2I DIR[4] = { {1,0}, {0,1}, {-1,0}, {0,-1} };

	for (int i = 0; i<WorldGrid::NUM_DEST; ++i) {
		Vector2I origin = s->core;

		if (i>0) {
			int port = (1 << (i - 1));
			if (!(s->ports & port)) {
				continue;
			}
		}

		switch (i) {
		case WorldGrid::PS_CORE:	/* nothing. already set.*/	break;
		case WorldGrid::PS_NEG_X:	origin = s->GetPortLoc(SectorData::NEG_X).Center();	break;
		case WorldGrid::PS_POS_X:	origin = s->GetPortLoc(SectorData::POS_X).Center();	break;
		case WorldGrid::PS_NEG_Y:	origin = s->GetPortLoc(SectorData::NEG_Y).Center();	break;
		case WorldGrid::PS_POS_Y:	origin = s->GetPortLoc(SectorData::POS_Y).Center();	break;
		default: GLASSERT(0);
		}
		Color( bounds, origin );

		for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
			for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
				Vector2I v = { x, y };

				if ( color[INDEX(v)] ) {
					// FIXME: this always chooses the first hit.
					// Would be nice to mix it up to make the
					// paths look more natural, and choose among
					// the valid paths.
					int dir=-1;
					if ( v == origin ) {
						dir = 0;
					}
					else {
						for( int k=0; k<4; ++k ) {
							Vector2I loc = v + DIR[k];
							if ( bounds.Contains( loc ) ) {
								int d = color[INDEX(v)] - color[INDEX(loc)];
								if ( d == 1 ) {
									dir = k;
									break;
								}
							}
						}
					}
					GLASSERT( dir >= 0 );
					path[INDEX(x,y)] |= dir << (i*2);
				}
			}
		}
	}
}




