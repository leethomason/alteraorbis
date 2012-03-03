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
	  targetWidth( 0 ),
	  targetHeight( 0 ),
	  format( RGBA16 ),
	  surface( 0 )
{
}


BTexture::~BTexture()
{
	if ( surface ) {
		SDL_FreeSurface( surface );
	}
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

	return true;
}


bool BTexture::Load()
{
	surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Failed to load surface." );
	}

	if (    isImage
		 || ( IsPowerOf2( surface->w ) && IsPowerOf2( surface-> h ) ) )
	{
		// no problem.
	}
	else {
		ExitError( "Texture", pathName.c_str(), assetName.c_str(), "Textures must be power of 2 dimension." );
	}

	printf( "%s Loaded: '%s' bpp=%d", 
			isImage ? "Image" : "Texture",
			assetName.c_str(),
			surface->format->BitsPerPixel );
	return true;
}


bool BTexture::Scale()
{
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

			// Bottom up!
			if ( !dither ) {
				for( int j=surface->h-1; j>=0; --j ) {
					for( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );

						U16 p =
								  ( ( c.r>>4 ) << 12 )
								| ( ( c.g>>4 ) << 8 )
								| ( ( c.b>>4 ) << 4)
								| ( ( c.a>>4 ) << 0 );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				OrderedDitherTo16( surface, RGBA16, true, &pixelBuffer16[0] );
			}
			break;

		case 24:
			printf( "  RGB memory=%dk\n", (surface->w * surface->h * 2)/1024 );

			// Bottom up!
			if ( !dither ) {
				for( int j=surface->h-1; j>=0; --j ) {
					for( int i=0; i<surface->w; ++i ) {
						Color4U8 c = GetPixel( surface, i, j );

						U16 p = 
								  ( ( c.r>>3 ) << 11 )
								| ( ( c.g>>2 ) << 5 )
								| ( ( c.b>>3 ) );

						pixelBuffer16.push_back(p);
					}
				}
			}
			else {
				pixelBuffer16.resize( surface->w*surface->h );
				pixelBuffer16.reserve( surface->w*surface->h );
				OrderedDitherTo16( surface, RGB16, true, &pixelBuffer16[0] );
			}
			break;

		case 8:
			printf( "  Alpha memory=%dk\n", (surface->w * surface->h * 1)/1024 );

			// Bottom up!
			for( int j=surface->h-1; j>=0; --j ) {
				for( int i=0; i<surface->w; ++i ) {
				    U8 *p = (U8 *)surface->pixels + j*surface->pitch + i;
					//SDL_RWwrite( fp, p, 1, 1 );
					pixelBuffer8.push_back(*p);
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

	if ( pixelBuffer16.size() ) {
		witem->SetData( "pixels", &pixelBuffer16[0], TextureMem() );
	}
	else if ( pixelBuffer8.size() ) {
		witem->SetData( "pixels", &pixelBuffer8[0], TextureMem() );
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
}