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

#include <stdlib.h>
#include "surface.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"
#include <string.h>
#include "serialize.h"

using namespace grinliz;

Surface::Surface() : format( TEX_RGB16 ), w( 0 ), h( 0 ), allocated( 0 ), pixels( 0 )
{
}

Surface::~Surface()
{
	delete [] pixels;
}


void Surface::SetName( const char* n )
{
	name = n;
}


void Surface::Set( TextureType f, int w, int h ) 
{
	this->format = f;
	this->w = w;
	this->h = h;
	
	int needed = w*h*BytesPerPixel();

	if ( allocated < needed ) {
		if ( pixels ) {
			delete [] pixels;
		}
		pixels = new U8[needed];
		allocated = needed;
	}
}


void Surface::Clear( int c )
{
	int needed = w*h*BytesPerPixel();
	memset( pixels, c, needed );
}


void Surface::BlitImg( const grinliz::Vector2I& target, const Surface* src, const grinliz::Rectangle2I& srcRect )
{
	GLASSERT( target.x >= 0 && target.y >= 0 );
	GLASSERT( target.x + srcRect.Width() <= w );
	GLASSERT( target.y + srcRect.Height() <= h );
	GLASSERT( srcRect.min.x >= 0 && srcRect.max.x < src->Width() );
	GLASSERT( srcRect.min.y >= 0 && srcRect.max.x < src->Height() );
	GLASSERT( src->format == format );

	const int scan = srcRect.Width() * BytesPerPixel();
	const int bpp = BytesPerPixel();

	for( int j=0; j<srcRect.Height(); ++j ) {
		memcpy( pixels + ((h-1)-(j+target.y))*w*bpp + target.x*bpp, 
				src->pixels + ((src->Height()-1)-(j+srcRect.min.y))*src->Width()*bpp + srcRect.min.x*bpp,
				scan );
	}
}


void Surface::BlitImg(	const grinliz::Rectangle2I& target, 
					const Surface* src, 
					const Matrix2I& xform )
{
	Vector3I t = { 0, 0, 1 };
	Vector3I s = { 0, 0, 1 };

	for( t.y=target.min.y; t.y<=target.max.y; ++t.y ) {
		for( t.x=target.min.x; t.x<=target.max.x; ++t.x ) {
			MultMatrix2I( xform, t, &s );		
			GLASSERT( s.x >= 0 && s.x < src->Width() );
			GLASSERT( s.y >= 0 && s.y < src->Height() );
			GLASSERT( BytesPerPixel() == 2 );	// just a bug...other case not implemented. Also need to fix origin.
			SetImg16( t.x, t.y, src->GetImg16( s.x, s.y ) );
		}
	}
}


/*static*/ TextureType Surface::QueryFormat( const char* formatString )
{
	if ( grinliz::StrEqual( "RGBA16", formatString ) ) {
		return TEX_RGBA16;
	}
	else if ( grinliz::StrEqual( "RGB16", formatString ) ) {
		return TEX_RGB16;
	}
	GLASSERT( 0 );
	return TEX_RGB16;
}


void Surface::ScaleByHalf()
{
	// Tricky. Modifies the buffer in-place.
	int halfW = w / 2;
	int halfH = h / 2;
	for( int y=0; y<halfH; ++y ) {
		for( int x=0; x<halfW; ++x ) {

			Color4U8 c00 = GetTex4U8( x*2+0, y*2+0 );
			Color4U8 c01 = GetTex4U8( x*2+0, y*2+1 );
			Color4U8 c10 = GetTex4U8( x*2+1, y*2+0 );
			Color4U8 c11 = GetTex4U8( x*2+1, y*2+1 );

			Color4U8 c = {  (c00.r+c01.r+c10.r+c11.r)>>2,
							(c00.g+c01.g+c10.g+c11.g)>>2,
							(c00.b+c01.b+c10.b+c11.b)>>2,
							(c00.a+c01.a+c10.a+c11.a)>>2 };

			int savedW = w;
			int savedH = h;
			w = halfW;
			h = halfH;
			SetTex4U8( x, y, c );
			w = savedW;
			h = savedH;
		}
	}
	Set( format, halfW, halfH );
}


void Surface::Load( const gamedb::Item* node )
{
	GLASSERT( node->GetBool( "isImage" ) == true );
	if ( !node->GetBool( "isImage" ) )
			return;

	name = node->Name();
	
	CStr<16> formatBuf = node->GetString( "format" ); 
	format = QueryFormat( formatBuf.c_str() );

	w = node->GetInt( "width" );
	h = node->GetInt( "height" );

	Set( Surface::QueryFormat( formatBuf.c_str() ), w, h );
		
	int size = node->GetDataSize( "pixels" );
	GLASSERT( size == BytesInImage() );

	node->GetData( "pixels", Pixels(), size );
}

void FlipSurface( void* pixels, int scanline, int h )
{
	U8* temp = new U8[scanline];

	for( int y=0; y<h/2; ++y ) {
		int b = h-1-y;

		memcpy( temp, (U8*)pixels + y*scanline, scanline );
		memcpy( (U8*)pixels + y*scanline, (U8*)pixels + b*scanline, scanline );
		memcpy( (U8*)pixels + b*scanline, temp, scanline );
	}

	delete [] temp;
}


ImageManager* ImageManager::instance = 0;

void ImageManager::Create(  const gamedb::Reader* db )
{
	GLASSERT( instance == 0 );
	instance = new ImageManager( db );
}


void ImageManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


void ImageManager::LoadImage( const char* name, Surface* surface )
{
	const gamedb::Item* textures = database->Root()->Child( "textures" );
	GLASSERT( textures );
	const gamedb::Item* item = textures->Child( name );
	GLASSERT( item );
	item = database->ChainItem( item ); 
	surface->Load( item );
}
