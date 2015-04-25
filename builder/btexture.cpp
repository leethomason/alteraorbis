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
#include "builder.h"
#include "../shared/lodepng.h"

using namespace grinliz;
using namespace tinyxml2;

extern void ExitError(	const char* tag, 
						const char* pathName,
						const char* assetName,
						const char* message );

extern Color4U8 GetPixel( const SDL_Surface *surface, int x, int y);
extern void PutPixel( SDL_Surface *surface, int x, int y, const Color4U8& c );

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);

bool BTexture::logToPNG = false;

BTexture::BTexture()
	: isImage( false ),
	  dither( false ),
	  noMip( false ),
	  colorMap( false ),
	  invert( true ),
	  emissive( false ),
	  alphaTexture(false),
	  whiteMap( false ),
	  targetWidth( 0 ),
	  targetHeight( 0 ),
	  targetMax( 0 ),
	  atlasX( 0 ),
	  atlasY( 0 ),
	  format( TEX_RGBA16 ),
	  surface( 0 ),
	  pixelBuffer( 0 )
{
}


BTexture::~BTexture()
{
	if ( surface ) {
		SDL_FreeSurface( surface );
	}
	delete [] pixelBuffer;
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
	element->QueryBoolAttribute( "colorMap", &colorMap );
	element->QueryIntAttribute( "width", &targetWidth );
	element->QueryIntAttribute( "height", &targetHeight );
	element->QueryIntAttribute( "maxSize", &targetMax );
	element->QueryBoolAttribute( "emissive", &emissive );
	element->QueryBoolAttribute("alphaTexture", &alphaTexture);
	element->QueryBoolAttribute( "whiteMap", &whiteMap );
	int depth = 16;
	element->QueryAttribute("depth", &depth);
	if (depth == 32) {
		format = TEX_RGBA32;
	}

	return true;
}


bool BTexture::Load()
{
	if ( alphaPathName.size() == 0 ) {
		surface = LoadImage( pathName.c_str() );
		if ( !surface ) {
			ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Failed to load surface." );
		}
	}
	else {
		// Seperate color and alpha textures
		SDL_Surface* rgb   = LoadImage( pathName.c_str() );
		SDL_Surface* alpha = LoadImage( alphaPathName.c_str() );
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
				Color4U8 c = { rgbC.r(), rgbC.g(), rgbC.b(), alphaC.r() };
				PutPixel( surface, x, y, c );
			}
		}
		SDL_FreeSurface( rgb );
		SDL_FreeSurface( alpha );
	}

	if (surface->format->BitsPerPixel == 24 || surface->format->BitsPerPixel == 32) {
	}
	else {
		ExitError("Texture", pathName.c_str(), assetName.c_str(), "Textures must be 24 or 32 bit.");
	}
		
		
	if (isImage || (IsPowerOf2(surface->w) && IsPowerOf2(surface->h))){
		// no problem.
	}
	else {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Textures must be power of 2 dimension." );
	}

	if ( surface->format->Amask )
		format = (TextureBytesPerPixel(format) == 2) ? TEX_RGBA16 : TEX_RGBA32;
	else 
		format = (TextureBytesPerPixel(format) == 2) ? TEX_RGB16  : TEX_RGB24;

	if (emissive && !TextureHasAlpha(format)) {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Emmisive only supported on RGBA." );
	}

	if (alphaTexture) {
		printf("Converting to alpha texture.\n");
		format = TEX_RGBA16;
		SDL_Surface* alpha = SDL_CreateRGBSurface(0, surface->w, surface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		for (int y = 0; y<surface->h; ++y) {
			for (int x = 0; x<surface->w; ++x) {
				Color4U8 rgb = GetPixel(surface, x, y);
				Color4U8 c = { 255, 255, 255, U8((rgb.x + rgb.y + rgb.z) / 3) };
				PutPixel(alpha, x, y, c);
			}
		}
		SDL_FreeSurface(surface);
		surface = alpha;
	}

	printf( "%s Loaded: '%s' bpp=%d em=%d output=%s", 
			isImage ? "Image" : "Texture",
			assetName.c_str(),
			surface->format->BitsPerPixel,
			emissive ? 1 : 0,
			TextureString(format));
	return true;
}


void BTexture::Create( int w, int h, TextureType format )
{
	this->targetWidth = w;
	this->targetHeight = h;
	this->format = format;

	GLASSERT( !surface );
	if ( TextureHasAlpha(format)) {
		surface = SDL_CreateRGBSurface( 0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
	}
	else {
		surface = SDL_CreateRGBSurface( 0, w, h, 24, 0xff0000, 0x00ff00, 0x0000ff, 0 );
	}
}


void BTexture::WhiteMap()
{
	// Create a new surface for the target.
	SDL_Surface* target = SDL_CreateRGBSurface( 0, surface->w, surface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
	format = TEX_RGBA16;

	for( int y=0; y<surface->h; ++y ) {
		for( int x=0; x<surface->w; ++x ) {
			Color4U8 c = GetPixel( surface, x, y );
			Color4U8 d = { c.r(), c.g(), c.b(), 0 };

			// Is it white? (Account for AA.)
			if (    c.b() == 0xff 
				 && c.r() > 0 
				 && c.g() > 0 
				 && abs( c.g() - c.r() ) < 2 ) 
			{
				int amount = c.r();
				float f = (float)amount / 255.0f;

				d.Set( 0,
					   0,
					   255,
					   (U8)Lerp( 0.0f, (float)c.r(), f  ));
			}
			PutPixel( target, x, y, d );
		}
	}
	SDL_FreeSurface( surface );
	surface = target;
}


bool BTexture::Scale()
{
	bool saveScaled = false;
	if ( whiteMap ) {
		saveScaled = true;
		WhiteMap();
	}
	if (alphaTexture) saveScaled = true;

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
		saveScaled = true;
	}

	if ( saveScaled ) {
		printf( " Scaled" );
		printf( " w=%d h=%d", surface->w, surface->h );
	}

	if ( logToPNG ) {
		GLString path = "./resin/scaled/";
		path += this->assetName;
		path += ".png";

		//SDL_SaveBMP( surface, path.c_str() );

		if ( surface->format->BytesPerPixel == 4 ) {
			int error = lodepng_encode_file( path.c_str(), (const U8*)surface->pixels, surface->w, surface->h, LCT_RGBA, 8 );
			if ( error )
				printf( "LodePNG error:  %s\n", lodepng_error_text( error ) );

		}
		else if ( surface->format->BytesPerPixel == 3 ) {
			int error = lodepng_encode_file( path.c_str(), (const U8*)surface->pixels, surface->w, surface->h, LCT_RGB, 8 );
			if ( error )
				printf( "LodePNG error:  %s\n", lodepng_error_text( error ) );
		}
		else if ( surface->format->BytesPerPixel == 1 ) {
			int error = lodepng_encode_file( path.c_str(), (const U8*)surface->pixels, surface->w, surface->h, LCT_GREY, 8 );
			if ( error )
				printf( "LodePNG error:  %s\n", lodepng_error_text( error ) );
		}

	}
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
					r32 += c.r();
					g32 += c.g();
					b32 += c.b();
					a32 += c.a();
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
	pixelBuffer = new U8[surface->w * surface->h * TextureBytesPerPixel(format)];

	if (format == TEX_RGB24 || format == TEX_RGBA32) {
		// The logic below assumes the pitch doesn't have padding, and that 
		// RGB is 3 bytes and RGBA is 4.
		GLASSERT(surface->pitch == surface->w * TextureBytesPerPixel(format));
		for (int j = 0; j<surface->h; ++j) {
			U8* p = pixelBuffer + j * surface->pitch;
			const U8* q = 0;
			if (invert)
				q = (U8*)surface->pixels + (surface->h - 1 - j) * surface->pitch;
			else
				q = (U8*)surface->pixels + j * surface->pitch;

			for (int i = 0; i<surface->pitch; ++i) {
				*p++ = *q++;
			}
		}
	}
	else if (format == TEX_RGBA16) {
		GLASSERT(surface->format->BitsPerPixel == 32);
		// Bottom up!
		if (!dither) {
			U16* b = (U16*)pixelBuffer;
			int start = invert ? surface->h - 1 : 0;
			int end = invert ? -1 : surface->h;
			int bias = invert ? -1 : 1;

			for (int j = start; j != end; j += bias) {
				for (int i = 0; i < surface->w; ++i) {
					Color4U8 c = GetPixel(surface, i, j);

					U16 p =
						((c.r() >> 4) << 12)
						| ((c.g() >> 4) << 8)
						| ((c.b() >> 4) << 4)
						| ((c.a() >> 4) << 0);

					*b++ = p;
				}
			}
		}
		else {
			OrderedDitherTo16(surface, TEX_RGBA16, invert, (U16*)pixelBuffer);
		}
	}
	else if (format == TEX_RGB16) {
		// Bottom up!
		if (!dither) {
			U16* b = (U16*)pixelBuffer;
			int start = invert ? surface->h - 1 : 0;
			int end = invert ? -1 : surface->h;
			int bias = invert ? -1 : 1;

			for (int j = start; j != end; j += bias) {
				for (int i = 0; i < surface->w; ++i) {
					Color4U8 c = GetPixel(surface, i, j);

					U16 p =
						((c.r() >> 3) << 11)
						| ((c.g() >> 2) << 5)
						| ((c.b() >> 3));

					*b++ = p;
				}
			}
		}
		else {
			OrderedDitherTo16(surface, TEX_RGB16, invert, (U16*)pixelBuffer);
		}
	}
	else {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Unsupported bit depth.\n" );
	}		
	printf("\n");
	return true;
}


gamedb::WItem* BTexture::InsertTextureToDB( gamedb::WItem* parent )
{
	gamedb::WItem* witem = parent->FetchChild( assetName.c_str() );

	witem->SetData("pixels", pixelBuffer, TextureMem());
	witem->SetString("format", TextureString(format));
	witem->SetBool( "isImage", isImage );
	witem->SetInt( "width", surface->w );
	witem->SetInt( "height", surface->h );
	witem->SetBool( "noMip", noMip );
	witem->SetBool( "colorMap", colorMap );
	witem->SetBool( "emissive", emissive );

	return witem;
}