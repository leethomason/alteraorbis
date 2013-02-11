#include "worldgen.h"

#include <string.h>

#include "../grinliz/glnoise.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"

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


/*
void WorldFeature::Save( gamedb::WItem* item )
{
	gamedb::WItem* f = item->FetchChild( "WorldFeature" );
	DB_SET( f, id );
	DB_SET( f, land );
	DB_SET( f, area );
	DB_SET( f, bounds );
}
*/


void WorldFeature::Serialize( DBItem parent, int i )
{
	DBItem f = DBChild( parent, i );
	DB_SERIAL( f, id );
	DB_SERIAL( f, land );
	DB_SERIAL( f, area );
	DB_SERIAL( f, bounds );
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
	memset( color, 0, SIZE*SIZE );
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
				float flix = flixels[j*SIZE+i];
				float f = Lerp( 1.0f, 255.0f, (flix-cutoff)/(maxh-cutoff) );
				int h = LRintf( f );
				GLASSERT( h >= 1 && h <= 255 );
//				if ( h == 255 )
//					int debug = 1;
				h = Clamp( h, 1, 255 );
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


bool WorldGen::CalColor( CDynArray<WorldFeature>* featureArr )
{
	memset( color, 0, SIZE*SIZE );
	int c = 1;

	Vector2I v;
	CDynArray<Vector2I> stack;
	Rectangle2I bounds;
	bounds.Set( 0, 0, SIZE-1, SIZE-1 );

	for( int j=0; j<SIZE; ++j ) {
		for( int i=0; i<SIZE; ++i ) {
			if ( color[j*SIZE+i] == 0 ) {
				bool isLand = land[j*SIZE+i] ? true : false;
				v.Set( i, j );
				stack.Push( v );

				WorldFeature wf;
				wf.id = c;
				wf.land = isLand ? true : false;
				wf.bounds.min = wf.bounds.max = v;
				wf.area = 0;

				while( !stack.Empty() ) {
					v = stack.Pop();
					if (    (color[v.y*SIZE+v.x] == 0)
						 && ((land[v.y*SIZE+v.x] > 0 ? true : false) == isLand) ) 
					{
						color[v.y*SIZE+v.x] = c;

						wf.bounds.DoUnion( v );
						wf.area++;
						
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
				++c;
				featureArr->Push( wf );
			}
		}
	}

	Sort<WorldFeature, WorldFeature::Compare>( featureArr->Mem(), featureArr->Size() );

	return c < 255;	// [0,254], need 255 for color-zone encoding in Save()
}


void WorldGen::DrawCanal( Vector2I v, int radius, int dx, int dy, const Rectangle2I& wf )
{
	Rectangle2I bounds;
	bounds.Set( 0, 0, SIZE-1, SIZE-1 );
	int bx = abs(dy)*radius;
	int by = abs(dx)*radius;
	Random random( v.x * v.y );
	
	while ( true ) {
		Rectangle2I brush;
		brush.Set( v.x-bx, v.y-by, v.x+bx, v.y+by );
		// Outside of world bounds, break.
		if ( !bounds.Contains( brush ) )
			break;
		// Outside of feature bounds and hit water, break.
		if ( !wf.Contains( brush ) && land[v.y*SIZE+v.x] == 0 )
			break;
		for( int y=brush.min.y; y<=brush.max.y; ++y ) {
			for( int x=brush.min.x; x<=brush.max.x; ++x ) {
				land[y*SIZE+x] = 0;
			}
		}
		v.x += dx;
		v.y += dy;

		if ( random.Rand(3) == 0 ) v.x += dy; 
		if ( random.Rand(3) == 0 ) v.x -= dy; 
		if ( random.Rand(3) == 0 ) v.y += dx; 
		if ( random.Rand(3) == 0 ) v.y -= dx; 
	}
}

