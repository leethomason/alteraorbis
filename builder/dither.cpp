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

#include "dither.h"
#include "builder.h"
#include "../grinliz/glutil.h"
#include "../engine/texturetype.h"

using namespace grinliz;

int ReducePixelDiv( int c, int shift, int num, int denom )
{
	int resolution = 1 << shift;	// the step size of color. So if the shift is 4, then the color resolation in 16

	// Increase cPrime by a number from [0,resolution)
	// The final step is to divide by the resolution,
	// so adding, on average, 1/2 the resolution to account
	// for the division truncating.
	//
	int cPrime = c + resolution*num/denom;	
	if ( cPrime > 255 ) cPrime = 255;

	int r = cPrime >> shift;
	return r;
}


//
// Ordered dithering:
// 1/5   1 3		(adds to 10)
//       4 2
//
// 1/17   1   9   3  11
//       13   5  15   7
//        4  12   2  10
//       16   8  14   6
//
// Originally the code was a diffusion-dither, which looked
// nice in the test images, but tended to created patterns
// that looked odd in the texture. The more regular appearence
// of the ordered dither looks better at run time.

void OrderedDitherTo16( const SDL_Surface* surface, TextureType format, bool invert, U16* target )
{
	// Use the dither pattern to both fix error and round up. Since the shift/division tends
	// to round the color down, the dither can diffuse and correct brightness errors.
	//
	//const int pattern[4] = { 1, 3, 4, 2 };
	//const int denom = 5;
	const int pattern[16] = { 1, 9, 3, 11, 13, 5, 15, 7, 4, 12, 2, 10, 16, 8, 14, 6 };
	const int denom = 17;

	for( int j0=0; j0<surface->h; ++j0 ) {
		int j = (invert) ? (surface->h-1-j0) : j0;

		for( int i=0; i<surface->w; ++i ) 
		{
			grinliz::Color4U8 c = GetPixel( surface, i, j );
			U16 p = 0;
			//int offset = (j&1)*2 + (i&1);
			int offset = (j&3)*4 + (i&3);
			const int numer = pattern[offset];

			switch ( format ) {
				case TEX_RGBA16:
					p =	  
						  ( ReducePixelDiv( c.r(), 4, numer, denom ) << 12 )
						| ( ReducePixelDiv( c.g(), 4, numer, denom ) << 8 )
						| ( ReducePixelDiv( c.b(), 4, numer, denom ) << 4)
						| ( ( c.a()>>4 ) << 0 );
					break;

				case TEX_RGB16:
					p = 
						  ( ReducePixelDiv( c.r(), 3, numer, denom ) << 11 )
						| ( ReducePixelDiv( c.g(), 2, numer, denom ) << 5 )
						| ( ReducePixelDiv( c.b(), 3, numer, denom ) );
					break;

				default:
					GLASSERT( 0 );
					break;
			}
			*target++ = p;
		}
	}
}


#if 0
// LIGHT/DARK offset error.
void DiffusionDitherTo16( const SDL_Surface* _surface, int format, bool invert, U16* target )
{
	SDL_Surface* surface = SDL_CreateRGBSurface( SDL_SWSURFACE, _surface->w, _surface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
	SDL_UpperBlit( (SDL_Surface*)_surface, 0, surface, 0 );

	for( int j0=0; j0<surface->h; ++j0 ) {
		int j = (invert) ? (surface->h-1-j0) : j0;

		for( int i=0; i<surface->w; ++i ) 
		{
			Color4U8 color = GetPixel( surface, i, j );
			U16 p = 0;
			Color4U8 colorPrime;

			int numer = 1;
			int denom = 1;

			switch ( format ) {
				case RGBA16:
					p =	  
						  ( ReducePixelDiv( color.r, 4, numer, denom ) << 12 )
						| ( ReducePixelDiv( color.g, 4, numer, denom ) << 8 )
						| ( ReducePixelDiv( color.b, 4, numer, denom ) << 4)
						| ( ( color.a>>4 ) << 0 );
					colorPrime = Convert_RGBA16_4U8( p );
					break;

				case RGB16:
					p = 
						  ( ReducePixelDiv( color.r, 3, numer, denom ) << 11 )
						| ( ReducePixelDiv( color.g, 2, numer, denom ) << 5 )
						| ( ReducePixelDiv( color.b, 3, numer, denom ) );
					colorPrime = Convert_RGB16_4U8( p );
					break;

				default:
					GLASSERT( 0 );
					break;
			}
			
			// Get the value we just wrote; distribute the error.
			Color4U8 error = color - colorPrime;

			// 0 * 7		1/16
			// 3 5 1
			static const Vector2I offset[4] = { {1,0}, {-1,1}, {0,1}, {1,1} };
			//static const float	  ratio[4]  = { 7.f, 3.f, 5.f, 1.f };
			static const int ratio[4] = { 7, 3, 5, 1 };
			for( int k=0; k<4; ++k ) {
				int x = i + offset[k].x;
				int y = j + offset[k].y;
				if ( x >= 0 && x<surface->w && y >= 0 && y < surface->h ) {
					Color4U8 c = GetPixel( surface, x, y );
					
					S8 er = error.r;
					S8 eg = error.g;
					S8 eb = error.b;

					c.r = Clamp( c.r + er*ratio[k]/10, 0, 255 );
					c.g = Clamp( c.g + eg*ratio[k]/10, 0, 255 );
					c.b = Clamp( c.b + eb*ratio[k]/10, 0, 255 );

					PutPixel( surface, x, y, c );
				}
			}
			

			*target++ = p;
		}
	}
	SDL_FreeSurface( surface );
}
#endif
