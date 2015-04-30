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
#include "../libs/SDL2/include/SDL.h"
#include "../engine/texturetype.h"
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
	gamedb::WItem* InsertTextureToDB( gamedb::WItem* parent );
	// ---- normal functions follow.
	void Create( int w, int h, TextureType format );

	int TextureMem() const {
		return surface->w * surface->h * TextureBytesPerPixel(format);
	}
	int PixelSize() const { return surface->w * surface->h; }

	static SDL_Surface* CreateScaledSurface( int w, int h, SDL_Surface* surface );

	grinliz::GLString pathName;
	grinliz::GLString alphaPathName;

	grinliz::GLString assetName;

	bool isImage;
	bool dither;
	bool noMip;
	bool colorMap;
	//bool doPreMult;
	bool invert;
	bool emissive;		// if set, the alpha channel is emissiveness, not transparency
	bool alphaTexture;	// if set, color converted to (1,1,1,c)
	bool whiteMap;
	int targetWidth;
	int targetHeight;
	int targetMax;
	int atlasX;
	int atlasY;

	static bool logToPNG;

	TextureType format;

	SDL_Surface* surface;
	U8* pixelBuffer;	// all formats!

private:	
	void WhiteMap();
};


#endif //  XENOENGINE_BTEXTURE_INCLUDED