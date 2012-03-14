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
		Tex tex( &array[i], array[i].assetName );
		textures.push_back( tex );
	}
	sort( textures.begin(), textures.end(), TexSorter );

	for( unsigned i=0; i<textures.size(); ++i ) {
		// Could patch up mip-mapping too. Until then:
		// 32x32 texture can only fall on 32 boundaries, etc.

		SDL_Surface* surface = textures[i].src->surface;
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

		textures[i].src->atlasX = bestX;
		textures[i].src->atlasY = bestY;

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

	for( unsigned i=0; i<textures.size(); ++i ) {
		printf( "-- %20s size=%d x=%d y=%d cx=%d cy=%d\n", 
			    textures[i].src->assetName.c_str(), 
				textures[i].src->PixelSize(), 
				textures[i].src->atlasX, textures[i].src->atlasY, 
				textures[i].src->surface->w, textures[i].src->surface->h );

		SDL_Rect src = { 0, 0, textures[i].src->surface->w, textures[i].src->surface->w };
		SDL_Rect dst = { textures[i].src->atlasX, textures[i].src->atlasY, textures[i].src->surface->w, textures[i].src->surface->w };
		SDL_BlitSurface( textures[i].src->surface, &src, btexture.surface, &dst );

		textures[i].x = textures[i].src->atlasX;
		textures[i].y = textures[i].src->atlasY;
		textures[i].cx = textures[i].src->surface->w;
		textures[i].cy = textures[i].src->surface->h;
		textures[i].src = 0;	// invalid after this call.
	}
	SDL_SaveBMP( btexture.surface, "atlas.bmp" );
	return 0;
}


bool Atlas::Map( const char* assetName, const grinliz::Vector2F& in, grinliz::Vector2F* out )
{
	for( unsigned i=0; i<textures.size(); ++i ) {
		if ( textures[i].assetName == assetName ) {

			int atlasCX = btexture.surface->w;
			int atlasCY = btexture.surface->h;
			int atlasX = textures[i].x;
			int atlasY = textures[i].y;
			int texCX = textures[i].cx;
			int texCY = textures[i].cy;

			out->x = ( (float)atlasX + in.x*(float)texCX ) / (float)atlasCX;
			out->y = ( (float)atlasY + in.y*(float)texCY ) / (float)atlasCY;
			return true;
		}
	}
	return false;
}

