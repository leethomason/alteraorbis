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
#include "../shared/dbhelper.h"

using namespace grinliz;

static const float BASE0 = 8.f;
static const float BASE1 = 64.f;
static const float EDGE = 0.1f;
static const float OCTAVE = 0.18f;


void WorldFeature::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "WorldFeature" );
	printer->PushAttribute( "id", id );
	printer->PushAttribute( "land", land );
	printer->PushAttribute( "bounds.min.x", bounds.min.x );
	printer->PushAttribute( "bounds.min.y", bounds.min.y );
	printer->PushAttribute( "bounds.max.x", bounds.max.x );
	printer->PushAttribute( "bounds.max.y", bounds.max.y );
	printer->PushAttribute( "area", area );
	printer->CloseElement();
}


void WorldFeature::Serialize( XStream* xs )
{
	XarcOpen( xs, "WorldFeature" );
	XARC_SER( xs, id );
	XARC_SER( xs, land );
	XARC_SER( xs, area );
	XARC_SER( xs, bounds );
	XarcClose( xs );
}


void WorldFeature::Load( const tinyxml2::XMLElement& ele )
{
	GLASSERT( StrEqual( ele.Name(), "WorldFeature" ));
	id = 0;
	land = 0;
	bounds.Set( 0, 0, 0, 0 );
	area = 0;

	ele.QueryIntAttribute( "id", &id );
	ele.QueryBoolAttribute( "land", &land );
	ele.QueryIntAttribute( "bounds.min.x", &bounds.min.x );
	ele.QueryIntAttribute( "bounds.min.y", &bounds.min.y );
	ele.QueryIntAttribute( "bounds.max.x", &bounds.max.x );
	ele.QueryIntAttribute( "bounds.max.y", &bounds.max.y );
	ele.QueryIntAttribute( "area", &area );
}


WorldGen::WorldGen()
{
	land = new U8[SIZE*SIZE];
	memset( land, 0, SIZE*SIZE );
	color = new U8[SIZE*SIZE];
	flixels = 0;
	noise0 = 0;
	noise1 = 0;
}


WorldGen::~WorldGen()
{
	delete [] land;
	delete [] color;
	delete [] flixels;
	delete noise0;
	delete noise1;
}


void WorldGen::StartLandAndWater( U32 seed0, U32 seed1 )
{
	flixels = new float[SIZE*SIZE];

	noise0 = new PerlinNoise( seed0 );
	noise1 = new PerlinNoise( seed1 );
}


void WorldGen::DoLandAndWater( int j )
{
	GLASSERT( j >= 0 && j < SIZE );
	for( int i=0; i<SIZE; ++i ) {
		float nx = (float)i/(float)SIZE;
		float ny = (float)j/(float)SIZE;

		// Noise layer.
		float n0 = noise0->Noise2( BASE0*nx, BASE0*ny );
		float n1 = noise1->Noise2( BASE1*nx, BASE1*ny );

		float n = n0 + n1*OCTAVE;
		n = PerlinNoise::Normalize( n );

		// Water at the edges.
		float dEdge = Min( nx, 1.0f-nx );
		if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );
		dEdge = Min( ny, 1.0f-ny );
		if ( dEdge < EDGE )		n = Lerp( 0.f, n, dEdge/EDGE );

		flixels[j*SIZE+i] = n;
	}
}


bool WorldGen::EndLandAndWater( float fractionLand )
{
	float cutoff = fractionLand;
	int target = (int)((float)(SIZE*SIZE)*fractionLand);
	float high = 1.0f;
	int highCount = 0;
	float low = 0.0f;
	int lowCount = SIZE*SIZE;
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

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			if ( flixels[j*SIZE+i] > cutoff ) {
				float flix = (flixels[j*SIZE+i] - cutoff) / (maxh-cutoff);
				GLASSERT( flix >= 0 && flix <= 1 );
				int h = LAND0 + (int)(flix*3.5f);
				GLASSERT( h > 0 && h <= 4 );
				land[j*SIZE+i] = h;
			}
			else {
				land[j*SIZE+i] = 0;
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
				land[y*SIZE+x] = 0;
			}
			else {
				land[y*SIZE+x] = 1;
			}
		}
	}
}


int WorldGen::CountFlixelsAboveCutoff( const float* flixels, float cutoff, float* maxh )
{
	int count = 0;
	*maxh = 0.0f;
	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			float f = flixels[j*SIZE+i];
			if ( f > cutoff ) {
				++count;
				if ( f > *maxh ) *maxh = f;
			}
		}
	}
	return count;
}


int WorldGen::CalcSectorAreaFromFill( SectorData* s, const grinliz::Vector2I& origin, bool* allPortsColored )
{
	memset( color, 0, SIZE*SIZE );
	int c = 1;

	CDynArray<Vector2I> stack;
	Rectangle2I bounds = s->InnerBounds();

	GLASSERT( land[origin.y*SIZE+origin.x] );

	stack.Push( origin );
	int area = 0;

	while( !stack.Empty() ) {
		Vector2I v = stack.Pop();
		if (    (color[v.y*SIZE+v.x] == 0)
			 && land[v.y*SIZE+v.x] ) 
		{
			color[v.y*SIZE+v.x] = 1;
			++area;

			Vector2I v0 = { v.x-1, v.y };
			Vector2I v1 = { v.x+1, v.y };
			Vector2I v2 = { v.x, v.y-1 };
			Vector2I v3 = { v.x, v.y+1 };

			if ( bounds.Contains( v0 ) ) stack.Push( v0 );
			if ( bounds.Contains( v1 ) ) stack.Push( v1 );
			if ( bounds.Contains( v2 ) ) stack.Push( v2 );
			if ( bounds.Contains( v3 ) ) stack.Push( v3 );
		}
	}
	if ( allPortsColored ) {
		*allPortsColored = true;
		for( int i=0; i<4; ++i ) {
			int port = 1<<i;
			if ( s->ports & port ) {
				Rectangle2I r = s->GetPortLoc( port );
				if ( color[r.min.y*SIZE+r.min.x] == 0 ) {
					*allPortsColored = false;
					break;
				}
			}
		}
	}
	return area;
}


void WorldGen::DepositLand( SectorData* s, U32 seed, int n )
{
	Rectangle2I bounds = s->InnerBounds();
	CDynArray<Vector2I> stack;

	// Inset one more, so that checks can be in all directions.
	for( int y=bounds.min.y+1; y<=bounds.max.y-1; ++y ) {
		for( int x=bounds.min.x+1; x<=bounds.max.x-1; ++x ) {
			if (    land[y*SIZE+x]		// is land
			     && (    land[y*SIZE+x+1] == 0
					  || land[y*SIZE+x-1] == 0
					  || land[(y+1)*SIZE+x] == 0
					  || land[(y-1)*SIZE+x] == 0 ))
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
		GLASSERT( stack.Size() < REGION_SIZE*REGION_SIZE );
		int d = Min( 4, stack.Size()-1 );
		int index = d > 1 ? (stack.Size()-1-random.Rand(d)) : (stack.Size()-1);
		Vector2I origin = stack[index];
		GLASSERT( land[origin.y*SIZE+origin.x] );

		bool deposit = false;
		for( int i=0; i<4; ++i ) {
			Vector2I c = origin + dir[i];
			if ( bounds.Contains(c) && land[c.y*SIZE+c.x] == 0 ) {
				land[c.y*SIZE+c.x] = LAND0;
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
	GLASSERT( r.min.x >= 0 && r.max.x < SIZE );
	GLASSERT( r.min.y >= 0 && r.max.y < SIZE );
	for( int y=r.min.y; y<=r.max.y; ++y ) {
		for( int x=r.min.x; x<=r.max.x; ++x ) {
			land[y*SIZE+x] = isLand;
		}
	}
}


void WorldGen::CutRoads( U32 seed, SectorData* sectorData )
{
	// Each sector is represented by one pather,
	// so they can't connect. First pass: split
	// up the world.
	for( int pass=0; pass<2; ++pass ) {
		for( int j=0; j<NUM_REGIONS-1; ++j ) {
			Rectangle2I r;
			r.Set( 0,	   (j+1)*REGION_SIZE-1, 
				   SIZE-1, (j+1)*REGION_SIZE );
			if ( pass == 1 ) {
				Swap( &r.min.x, &r.min.y );
				Swap( &r.max.x, &r.max.y );
			}

			Draw( r, 0 );
		}
	}

	// Set which sectors are on the road. When roads are
	// everything which can be a road is a road.
	BitArray< NUM_REGIONS, NUM_REGIONS, 1 > sectors;
	Random random( seed );

	Vector2I center = { NUM_REGIONS/2, NUM_REGIONS/2 };
	int rad = NUM_REGIONS/2;

	for( int j=1; j<NUM_REGIONS-2; ++j ) {
		int dy = j - center.x;
		int d2 = rad*rad - dy*dy;
		int dx = d2 ? LRintf( sqrt( (float)(d2) )) : 1;

		int dx0 = random.Rand( dx )/2 + dx/2;
		int dx1 = random.Rand( dx )/2 + dx/2;

		if ( dx0 == 0 && dx1 == 0 ) {
			if ( random.Bit() )
				dx0 = 1;
			else
				dx1 = 1;
		}

		int x0 = center.x - dx0;
		int x1 = center.x + dx1;
		x0 = Clamp( x0, 1, NUM_REGIONS-2 );
		x1 = Clamp( x1, 1, NUM_REGIONS-2 );

		Rectangle2I r;
		r.Set( x0, j, x1, j );
		sectors.SetRect( r );
	}

	// Now fill in roads.
	for( int j=1; j<NUM_REGIONS-1; ++j ) {
		for( int i=1; i<NUM_REGIONS-1; ++i ) {
			if ( sectors.IsSet( i, j )) {
				Rectangle2I r;

				int x0 = i*REGION_SIZE;
				int x1 = (i+1)*REGION_SIZE-1;
				int y0 = j*REGION_SIZE;
				int y1 = (j+1)*REGION_SIZE-1;

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
					sectorData[j*NUM_REGIONS+i].ports |= SectorData::NEG_X;
				}
				if ( right ) {
					r.Set( x1, y0+1, x1+1, y1+1 );
					Draw( r, GRID );
					sectorData[j*NUM_REGIONS+i].ports |= SectorData::POS_X;
				}
				if ( up ) {
					r.Set( x0-1, y0-1, x1+1, y0 );
					Draw( r, GRID );
					sectorData[j*NUM_REGIONS+i].ports |= SectorData::NEG_Y;
				}
				if ( down ) {
					r.Set( x0-1, y1, x1+1, y1+1 );
					Draw( r, GRID );
					sectorData[j*NUM_REGIONS+i].ports |= SectorData::POS_Y;
				}
			}
		}
	}
}


int WorldGen::CalcSectorArea( int sx, int sy )
{
	int area = 0;
	for( int j=sy*REGION_SIZE+1;
		 j < (sy+1)*REGION_SIZE-1;
		 ++j )
	{
		for( int i=sx*REGION_SIZE+1;
			 i < (sx+1)*REGION_SIZE-1;
			 ++i )
		{
			if ( land[j*SIZE+i] ) {
				++area;
			}
		}
	}
	return area;
}


Rectangle2I SectorData::Bounds() { 
	grinliz::Rectangle2I r; 
	r.Set( x, y, x+WorldGen::REGION_SIZE-1, y+WorldGen::REGION_SIZE-1 ); 
	return r; 
}


Rectangle2I SectorData::InnerBounds() { 
	grinliz::Rectangle2I r = Bounds();
	r.Outset( -1 );
	return r;
}

Rectangle2I SectorData::GetPortLoc( int port )
{
	Rectangle2I r;
	static const int LONG  = 4;
	static const int REGION_SIZE = WorldGen::REGION_SIZE;
	static const int OFFSET = (REGION_SIZE-LONG)/2;
	static const int SHORT = 2;

	if ( port == NEG_X ) {
		r.Set( x+1, y+OFFSET, x+SHORT, y+OFFSET+LONG-1 ); 
	}
	else if ( port == POS_X ) {
		r.Set( x+REGION_SIZE-SHORT-1, y+OFFSET, x+REGION_SIZE-2, y+OFFSET+LONG-1 ); 
	}
	else if ( port == NEG_Y ) {
		r.Set( x+OFFSET, y+1, x+OFFSET+LONG-1, y+SHORT ); 
	}
	else if ( port == SectorData::POS_Y ) {
		r.Set( x+OFFSET, y+REGION_SIZE-SHORT-1, x+OFFSET+LONG-1, y+REGION_SIZE-2 ); 
	}
	else {
		GLASSERT( 0 );
	}
	return r;
}


void WorldGen::AddPorts( SectorData* s )
{
	int x = s->x;
	int y = s->y;

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
	// Calc the area.
	// Sort by area.
	// Choose N and fill in for cores.

	static const int NCORES = 100;
	CDynArray<SectorData*> sectors;

	for( int j=0; j<NUM_REGIONS; ++j ) {
		for( int i=0; i<NUM_REGIONS; ++i ) {
			SectorData* s = &sectorData[j*NUM_REGIONS+i];
			s->x = i*REGION_SIZE;
			s->y = j*REGION_SIZE;

			if ( s->ports ) {
				AddPorts( s );
				sectors.Push( s );
			}
			s->area = CalcSectorArea( i, j );
		}
	}

	Sort< SectorData*, SectorPtrArea >( sectors.Mem(), sectors.Size() );
	for( int i=0; i<sectors.Size() && i<NCORES; ++i ) {
		GenerateTerrain( seed+i, sectors[i] );
	}
}


void WorldGen::GenerateTerrain( U32 seed, SectorData* s )
{
	static const int AREA = INNER_REGION_SIZE*INNER_REGION_SIZE / 2;

	Random random( seed );

	// Place core
	Vector2I c = {	s->x + REGION_SIZE/2 - 10 + random.Dice( 3, 6 ),
					s->y + REGION_SIZE/2 - 10 + random.Dice( 3, 6 )  };

	Rectangle2I r;
	r.max = r.min = c;
	r.Outset( 2 );

	Draw( r, LAND0 );
	land[c.y*SIZE+c.x] = CORE;

	bool portsColored = false;
	int a = CalcSectorAreaFromFill( s, c, &portsColored );
	while ( a < AREA || !portsColored ) {
		DepositLand( s, seed, Max( AREA-a, (int)INNER_REGION_SIZE ));
		a = CalcSectorAreaFromFill( s, c, &portsColored );
	}
	s->area = CalcSectorArea( s->x/REGION_SIZE, s->y/REGION_SIZE );
}


