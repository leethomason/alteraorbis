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

#include "btexture.h"
#include "../grinliz/glcolor.h"
#include "dither.h"

using namespace grinliz;
using namespace tinyxml2;

extern void ExitError(	const char* tag, 
						const char* pathName,
						const char* assetName,
						const char* message );

extern Color4U8 GetPixel( const SDL_Surface *surface, int x, int y);
extern void PutPixel( SDL_Surface *surface, int x, int y, const Color4U8& c );

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
extern PFN_IMG_LOAD libIMG_Load;



BTexture::BTexture()
	: isImage( false ),
	  dither( false ),
	  noMip( false ),
	  doPreMult( false ),
	  invert( true ),
	  emissive( false ),
	  blackAlpha0( false ),
	  targetWidth( 0 ),
	  targetHeight( 0 ),
	  targetMax( 0 ),
	  atlasX( 0 ),
	  atlasY( 0 ),
	  format( RGBA16 ),
	  surface( 0 ),
	  pixelBuffer16( 0 ),
	  pixelBuffer8( 0 )
{
}


BTexture::~BTexture()
{
	if ( surface ) {
		SDL_FreeSurface( surface );
	}
	delete [] pixelBuffer16;
	delete [] pixelBuffer8;
}


bool BTexture::ParseTag( const tinyxml2::XMLElement* element )
{
	if ( StrEqual( element->Value(), "image" )) {
		isImage = true;
	}
	else if ( StrEqual( element->Value(), "texture" )) {
		isImage = false;
	}
	else if ( StrEqual( element->Value(), "font" )) {
		isImage = false;
	}
	else {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "incorrect tag." );
	}
	element->QueryBoolAttribute( "dither", &dither );
	element->QueryBoolAttribute( "noMip", &noMip );
	element->QueryIntAttribute( "width", &targetWidth );
	element->QueryIntAttribute( "height", &targetHeight );
	element->QueryIntAttribute( "maxSize", &targetMax );
	element->QueryBoolAttribute( "premult", &doPreMult );
	element->QueryBoolAttribute( "emissive", &emissive );
	element->QueryBoolAttribute( "blackAlpha0", &blackAlpha0 );

	return true;
}


bool BTexture::Load()
{
	if ( alphaPathName.size() == 0 ) {
		surface = libIMG_Load( pathName.c_str() );
		if ( !surface ) {
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Failed to load surface." );
		}
	}
	else {
		// Seperate color and alpha textures
		SDL_Surface* rgb = libIMG_Load( pathName.c_str() );
		SDL_Surface* alpha = libIMG_Load( alphaPathName.c_str() );
		if ( !rgb ) {
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Failed to load RGB surface." );
		}
		if ( !alpha ) {
			ExitError( "Texture", alphaPathName.c_str(), assetName.c_str(), "Failed to load Alpha surface." );
		}
		if ( rgb->w != alpha->w || rgb->h != alpha->h ) {
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "RGB and Alpha textures are different sizes." );
		}
		surface = SDL_CreateRGBSurface( 0, rgb->w, rgb->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
		for( int y=0; y<rgb->h; ++y ) {
			for( int x=0; x<rgb->w; ++x ) {
				Color4U8 rgbC = GetPixel( rgb, x, y );
				Color4U8 alphaC = GetPixel( alpha, x, y );
				Color4U8 c = { rgbC.r, rgbC.g, rgbC.b, alphaC.r };
				PutPixel( surface, x, y, c );
			}
		}
		SDL_FreeSurface( rgb );
		SDL_FreeSurface( alpha );
	}

	if (    isImage
		 || ( IsPowerOf2( surface->w ) && IsPowerOf2( surface-> h ) ) )
	{
		// no problem.
	}
	else {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Textures must be power of 2 dimension." );
	}

	if (    surface->format->BitsPerPixel == 24
		 || surface->format->BitsPerPixel == 32 )	
	{
		if ( surface->format->Amask )
			format = RGBA16;
		else 
			format = RGB16;
	}
	else if ( surface->format->BitsPerPixel == 8 ) {
		format = ALPHA;
	}
	else {
		GLASSERT( 0 );
	}

	if ( emissive && format != RGBA16 ) {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Emmisive only supported on RGBA." );
	}
	if ( format == RGBA16 && blackAlpha0 ) {
		for( int y=0; y<surface->h; ++y ) {
			for( int x=0; x<surface->w; ++x ) {
				Color4U8 rgbC = GetPixel( surface, x, y );
				if ( rgbC.a == 0 ) {
					rgbC.Set( 0, 0, 0, 0 );
				}
				Color4U8 c = { rgbC.r, rgbC.g, rgbC.b, rgbC.a };
				PutPixel( surface, x, y, c );
			}
		}

	}

	printf( "%s Loaded: '%s' bpp=%d em=%d", 
			isImage ? "Image" : "Texture",
			assetName.c_str(),
			surface->format->BitsPerPixel,
			emissive ? 1 : 0 );
	return true;
}


void BTexture::Create( int w, int h, int format )
{
	this->targetWidth = w;
	this->targetHeight = h;
	this->format = format;

	GLASSERT( !surface );
	if ( format == RGBA16 ) {
		surface = SDL_CreateRGBSurface( 0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
	}
	else if ( format == RGB16 ) {
		surface = SDL_CreateRGBSurface( 0, w, h, 24, 0xff0000, 0x00ff00, 0x0000ff, 0 );
	}
	else if ( format == ALPHA ) {
		surface = SDL_CreateRGBSurface( 0, w, h, 8, 0, 0, 0, 0xff );
	}
}


bool BTexture::Scale()
{
	if ( targetMax ) {
		targetWidth = surface->w;
		targetHeight = surface->h;
		while ( targetWidth > targetMax ) {
			targetWidth >>= 1;
			targetHeight >>= 1;
		}
		while ( targetHeight > targetMax ) {
			targetWidth >>= 1;
			targetHeight >>= 1;
		}
	}

	if ( targetWidth && targetHeight )
	{
		SDL_Surface* old = surface;
		surface = CreateScaledSurface( targetWidth, targetHeight, surface );
		SDL_FreeSurface( old );

		printf( " Scaled" );
	}
	printf( " w=%d h=%d", surface->w, surface->h );
	return true;
}


SDL_Surface* BTexture::CreateScaledSurface( int w, int h, SDL_Surface* surface )
{
	int sx = surface->w / w;
	int sy = surface->h / h;

	GLASSERT( sx > 0 && sy > 0 );
	SDL_Surface* newSurf = SDL_CreateRGBSurface( 0, w, h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );

	for( int y=0; y<h; ++y ) {
		for( int x=0; x<w; ++x ) {
			int r32=0, g32=0, b32=0, a32=0;
			
			int xSrc = x*sx;
			int ySrc = y*sy;
			for( int j=0; j<sy; ++j ) {
				for( int i=0; i<sx; ++i ) {
					Color4U8 c = GetPixel( surface, xSrc+i, ySrc+j );
					r32 += c.r;
					g32 += c.g;
					b32 += c.b;
					a32 += c.a;
				}
			}
			Color4U8 c = { r32/(sx*sy), g32/(sx*sy), b32/(sx*sy), a32/(sx*sy) };
			PutPixel( newSurf, x, y, c );
		}
	}
	return newSurf;
}


bool BTexture::ToBuffer()
{
	switch( surface->format->BitsPerPixel ) {
		case 32:
			printf( "  RGBA memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			pixelBuffer16 = new U16[ surface->w * surface->h ];

			if ( doPreMult ) {
				for( int j=0; j<surface->h; ++j ) {
					for ( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );
						c.r = c.r*c.a/255;
						c.g = c.g*c.a/255;
						c.b = c.b*c.a/255;
						PutPixel( surface, i, j, c );
					}
				}

			}

			// Bottom up!
			if ( !dither ) {
				U16* b = pixelBuffer16;
				int start = invert ? surface->h-1 : 0;
				int end =   invert ? -1 : surface->h;
				int bias =  invert ? -1 : 1;

				for( int j=start; j != end; j+= bias ) {
					for( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );

						U16 p =
								  ( ( c.r>>4 ) << 12 )
								| ( ( c.g>>4 ) << 8 )
								| ( ( c.b>>4 ) << 4)
								| ( ( c.a>>4 ) << 0 );

						*b++ = p;
					}
				}
			}
			else {
				OrderedDitherTo16( surface, RGBA16, invert, pixelBuffer16 );
			}
			break;

		case 24:
			printf( "  RGB memory=%dk\n", (surface->w * surface->h * 2)/1024 );
			pixelBuffer16 = new U16[ surface->w * surface->h ];

			// Bottom up!
			if ( !dither ) {
				U16* b = pixelBuffer16;
				int start = invert ? surface->h-1 : 0;
				int end =   invert ? -1 : surface->h;
				int bias =  invert ? -1 : 1;

				for( int j=start; j != end; j+= bias ) {
					for( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );

						U16 p = 
								  ( ( c.r>>3 ) << 11 )
								| ( ( c.g>>2 ) << 5 )
								| ( ( c.b>>3 ) );

						*b++ = p;
					}
				}
			}
			else {
				OrderedDitherTo16( surface, RGB16, invert, pixelBuffer16 );
			}
			break;

		case 8:
			{
			printf( "  Alpha memory=%dk\n", (surface->w * surface->h * 1)/1024 );
			pixelBuffer8 = new U8[ surface->w * surface->h ];
			U8* b = pixelBuffer8;

			// Bottom up!
			int start = invert ? surface->h-1 : 0;
			int end =   invert ? -1 : surface->h;
			int bias =  invert ? -1 : 1;

			for( int j=start; j != end; j+= bias ) {
				for( int i=0; i<surface->w; ++i ) {
				    U8 *p = (U8 *)surface->pixels + j*surface->pitch + i;
					*b++ = *p;
				}
			}
			}
			break;

		default:
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Unsupported bit depth.\n" );
			break;
	}		
	return true;
}


void BTexture::InsertTextureToDB( gamedb::WItem* parent )
{
	gamedb::WItem* witem = parent->CreateChild( assetName.c_str() );

	if ( format == RGBA16 || format == RGB16 ) {
		witem->SetData( "pixels", pixelBuffer16, TextureMem() );
	}
	else if ( format == ALPHA ) {
		witem->SetData( "pixels", pixelBuffer8, TextureMem() );
	}
	else {
		GLASSERT( 0 );
	}

	switch( format ) {
	case RGBA16:	witem->SetString( "format", "RGBA16" );	break;
	case RGB16:		witem->SetString( "format", "RGB16" );	break;
	case ALPHA:		witem->SetString( "format", "ALPHA" );	break;
	default:	GLASSERT( 0 );
	}

	witem->SetBool( "isImage", isImage );
	witem->SetInt( "width", surface->w );
	witem->SetInt( "height", surface->h );
	witem->SetBool( "noMip", noMip );
	witem->SetBool( "emissive", emissive );
}