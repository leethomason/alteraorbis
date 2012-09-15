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


#include "atlas.h"
#include "../grinliz/glutil.h"
#include <limits.h>

using namespace std;
using namespace grinliz;


extern void ExitError( const char* tag, 
				const char* pathName,
				const char* assetName,
				const char* message );

Atlas::Atlas()
{
}


Atlas::~Atlas()
{
}


SDL_Surface* Atlas::Generate( BTexture* array, int nTexture, int maxWidth )
{
	stack.resize( maxWidth );

	for( int i=0; i<nTexture; ++i ) {
		if ( array[i].format != array[0].format ) {
			ExitError( "Atlas", 0, 0, "mis matched texture formats" );
		}
		AtlasSubTex tex( &array[i], array[i].assetName );
		subTexArr.push_back( tex );
	}
	sort( subTexArr.begin(), subTexArr.end(), TexSorter );

	for( unsigned i=0; i<subTexArr.size(); ++i ) {
		// Could patch up mip-mapping too. Until then:
		// 32x32 texture can only fall on 32 boundaries, etc.

		SDL_Surface* surface = subTexArr[i].src->surface;
		int cx = surface->w;
		int cy = surface->h;
		GLASSERT( IsPowerOf2( cx ) );
		GLASSERT( IsPowerOf2( cy ) );
		
		int bestY = INT_MAX;
		int bestX = 0;

		for( int x=0; x+cx <= maxWidth; x += cx ) {
			// find the max y in that range:
			int maxY = *max_element( &stack[x], &stack[x+cx-1] ); // Amailia's contribution: r
			if ( maxY < bestY ) {
				bestY = maxY;
				bestX = x;
			}
		}
		// adjust bestY for actual start:
		bestY = ((bestY+cy-1)/cy)*cy;

		subTexArr[i].src->atlasX = bestX;
		subTexArr[i].src->atlasY = bestY;

		// write back to the stack:
		for( int x=bestX; x < bestX+cx; ++x ) {
			stack[x] = bestY+cy;
		}
	}

	printf( "\n" );

	int maxY = *max_element( &stack[0], &stack[maxWidth-1] );
	maxY = CeilPowerOf2( maxY );
	printf( "Atlas %dx%d\n", maxWidth, maxY );

	btexture.Create( maxWidth, maxY, array[0].format );

	for( unsigned i=0; i<subTexArr.size(); ++i ) {
		SDL_Rect src = { 0, 0, subTexArr[i].src->surface->w, subTexArr[i].src->surface->h };
		SDL_Rect dst = { subTexArr[i].src->atlasX, subTexArr[i].src->atlasY, subTexArr[i].src->surface->w, subTexArr[i].src->surface->h };
		SDL_BlitSurface( subTexArr[i].src->surface, &src, btexture.surface, &dst );

		subTexArr[i].x  = subTexArr[i].src->atlasX;
		subTexArr[i].y  = subTexArr[i].src->atlasY;
		subTexArr[i].cx = subTexArr[i].src->surface->w;
		subTexArr[i].cy = subTexArr[i].src->surface->h;
		subTexArr[i].atlasCX = maxWidth;
		subTexArr[i].atlasCY = maxY;
		subTexArr[i].src = 0;	// invalid after this call.
	}

	printf( "Atlas: '%s' %dx%d\n", btexture.assetName.c_str(), btexture.surface->w, btexture.surface->h );
	for( unsigned i=0; i<subTexArr.size(); ++i ) {
		printf( "  %20s x=%d y=%d cx=%d cy=%d\n", 
				subTexArr[i].assetName.c_str(),
				subTexArr[i].x,
				subTexArr[i].y,
				subTexArr[i].cx, 
				subTexArr[i].cy );
	}

	SDL_SaveBMP( btexture.surface, "atlas.bmp" );
	return 0;
}


const AtlasSubTex* Atlas::GetSubTex( const char* assetName ) const
{
	// Only have to match until '.'
	for( unsigned i=0; i<subTexArr.size(); ++i ) {
		if ( StrEqualUntil( subTexArr[i].assetName.c_str(), assetName, '.' )) {
			return &subTexArr[i];
		}
	}
	return 0;
}



void AtlasSubTex::Map( const grinliz::Vector2F& in, grinliz::Vector2F* out ) const
{
	GLASSERT( in.x >= -0.01f && in.x <= 1.01f );
	GLASSERT( in.y >= -0.01f && in.y <= 1.01f );

	float inx = Clamp( in.x, 0.0f, 1.0f );
	float iny = Clamp( in.y, 0.0f, 1.0f );


	out->x = ( (float)x + inx*(float)cx ) / (float)atlasCX;
	//out->y = 1.f - ( (float)y + iny*(float)cy ) / (float)atlasCY;
	out->y = (1.0f-(float)(y+cy)/(float)atlasCY) + iny*(float)cy / (float)atlasCY;

	GLASSERT( out->x >= -0.0f && out->x <= 1.0f );
	GLASSERT( out->y >= -0.0f && out->y <= 1.0f );
}

