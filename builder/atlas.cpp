#include "atlas.h"
#include "../grinliz/glutil.h"
#include <limits.h>

using namespace std;
using namespace grinliz;


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
		SDL_Rect src = { 0, 0, subTexArr[i].src->surface->w, subTexArr[i].src->surface->w };
		SDL_Rect dst = { subTexArr[i].src->atlasX, subTexArr[i].src->atlasY, subTexArr[i].src->surface->w, subTexArr[i].src->surface->w };
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
	out->x = ( (float)x + in.x*(float)cx ) / (float)atlasCX;
	out->y = ( (float)y + in.y*(float)cy ) / (float)atlasCY;
}

