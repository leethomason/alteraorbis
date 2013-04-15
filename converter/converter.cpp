#include "SDL.h"

#include <stdio.h>
#include <stdlib.h>
#include <set>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gldynamic.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glrandom.h"

#include "../shared/lodepng.h"

using namespace std;
using namespace grinliz;

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

U8 mix( U8 v0, U8 v1, U8 a ) {
	int v = (v0*(255-a) + v1*a)/255;
	GLASSERT( v >= 0 && v < 256 );
	return (U8)v;
}


Color4U8 mix( Color4U8 v0, Color4U8 v1, U8 a ) {
	Color4U8 c;
	c.r = mix( v0.r, v1.r, a );
	c.g = mix( v0.g, v1.g, a );
	c.b = mix( v0.b, v1.b, a );
	c.a = Max( v0.a, v1.a, a );
	return c;
}

Color4U8 GetPixel( const SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	Color4U8 color = { p[0], p[1], p[2], p[3] };
	return color;
}


void PutPixel(SDL_Surface *surface, int x, int y, const Color4U8& c )
{
    U8 *p = (U8*)surface->pixels + y * surface->pitch + x * 4;
	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
	p[3] = c.a;
}


int main( int argc, char* argv[] )
{
	static const int NUM_IMAGES = 120;
	static const char* SRC_PATH = "../resin/humanMaleFace.png";
	static const int GLASSES_START = 1;
	static const int GLASSES_END   = 11;

	void *handle = 0;
	handle = grinliz::grinlizLoadLibrary( "SDL_image" );
	if ( !handle )
	{	
		printf( "Could not load SDL_image library.\n" );
		exit( 1 );
	}
	libIMG_Load = (PFN_IMG_LOAD) grinliz::grinlizLoadFunction( handle, "IMG_Load" );
	GLASSERT( libIMG_Load );

	SDL_Surface* surface = libIMG_Load( SRC_PATH );
	GLASSERT( surface );
	if ( !surface ) exit(1);
	SDL_Surface* outSurf = SDL_CreateRGBSurface( SDL_SWSURFACE, 256, 256, 32, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );

	std::set<int> outset;

	int row0 = 0;
	int row1 = 0;
	int row2 = 0;
	int row3 = 0;
	int rows = surface->h / 256;
	Random random;

	for( int n=0; n<NUM_IMAGES; ++n ) {
		int row0 = n % rows;

		while(true) {
			row1 = random.Rand( rows );
			row2 = 0;
			if ( random.Bit() ) {
				row2 = GLASSES_START + random.Rand( GLASSES_END - GLASSES_START  );
			}
			row3 = random.Rand( rows );

			int hash = row0 | (row1<<8) | (row2<<16) | (row3<<24);
			if ( outset.find( hash ) == outset.end() ) {
				outset.insert( hash );
				break;
			}
		}

		printf( "Face %2d row=%d %d %d %d\n", n, row0, row1, row2, row3 );

		for( int j=0; j<256; ++j ) {
			for( int i=0; i<256; ++i ) {
				
				Color4U8 cIn[4];
				cIn[0] = GetPixel( surface, i,     j+row0*256 );
				cIn[1] = GetPixel( surface, i+256, j+row1*256 );
				cIn[2] = GetPixel( surface, i+512, j+row2*256 );
				cIn[3] = GetPixel( surface, i+768, j+row3*256 );

				Color4U8 c = { 0,0,0,0 };

				for( int k=0; k<4; ++k ) {
					c = mix( c, cIn[k], cIn[k].a );
				}

				PutPixel( outSurf, i, j, c );
			}
		}

		CStr<200> buf;
		buf.Format( "./files/out%03d.png", n );
		lodepng_encode32_file( buf.c_str(), (const unsigned char*)outSurf->pixels, outSurf->w, outSurf->h );
	}
	printf( "goodbye\n" );
	return 0;
}

