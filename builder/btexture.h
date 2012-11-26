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

#ifndef XENOENGINE_BTEXTURE_INCLUDED
#define XENOENGINE_BTEXTURE_INCLUDED

#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glstringutil.h"
#include "../shared/gamedbwriter.h"
#include "SDL.h"
#include <vector>

class BTexture
{
public:
	BTexture();
	~BTexture();

	// -- The "do in order" block.
	bool ParseTag( const tinyxml2::XMLElement* element );
	void SetNames(	const grinliz::GLString& asset, 
					const grinliz::GLString& path,
					const grinliz::GLString& path2)	
	{ 
		this->assetName = asset; 
		this->pathName = path; 
		this->alphaPathName = path2;
	}
	bool Load();		// do the load (can use create instad)
	bool Scale();		// scale, if targetW/H not asset W/H
	bool ToBuffer();	// dither, flip, process, etc. to memory buffer
	void InsertTextureToDB( gamedb::WItem* parent );
	// ---- normal functions follow.
	void Create( int w, int h, int format );

	int TextureMem() const {
		if ( format == RGB16 || format == RGBA16 ) return (surface->w * surface->h * 2);
		if ( format == ALPHA )  return (surface->w * surface->h);
		GLASSERT( 0 );
		return 0;
	}
	int PixelSize() const { return surface->w * surface->h; }

	static SDL_Surface* BTexture::CreateScaledSurface( int w, int h, SDL_Surface* surface );

	grinliz::GLString pathName;
	grinliz::GLString alphaPathName;

	grinliz::GLString assetName;

	bool isImage;
	bool dither;
	bool noMip;
	bool doPreMult;
	bool invert;
	bool emissive;	// if set, the alpha channel is emissiveness, not transparency
	bool blackAlpha0;	// if set, all alpha=0 becomes 0,0,0,0	
	int targetWidth;
	int targetHeight;
	int targetMax;
	int atlasX;
	int atlasY;

	enum {
		RGBA16,		// 4444		2
		RGB16,		// 565		2
		ALPHA,		// 8		1
	};
	int format;

	SDL_Surface* surface;
	U16* pixelBuffer16;	// RGBA16, RGB16
	U8*  pixelBuffer8;	// ALPHA
};


#endif //  XENOENGINE_BTEXTURE_INCLUDED