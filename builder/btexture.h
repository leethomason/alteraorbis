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

	bool ParseTag( const tinyxml2::XMLElement* element );
	void SetNames( const grinliz::GLString& asset, const grinliz::GLString& path )	{ this->assetName = asset; this->pathName = path; }
	bool Load();		// do the load
	bool Scale();		// scale, if targetW/H not asset W/H
	bool ToBuffer();	// dither, flip, process, etc. to memory buffer
	void InsertTextureToDB( gamedb::WItem* parent );

	int TextureMem() const {
		if ( format == RGB16 || format == RGBA16 ) return (surface->w * surface->h * 2);
		if ( format == ALPHA )  return (surface->w * surface->h);
		GLASSERT( 0 );
		return 0;
	}

	static SDL_Surface* BTexture::CreateScaledSurface( int w, int h, SDL_Surface* surface );

	grinliz::GLString pathName;
	grinliz::GLString assetName;

	bool isImage;
	bool dither;
	bool noMip;
	int targetWidth;
	int targetHeight;

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