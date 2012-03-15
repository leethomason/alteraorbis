#ifndef XENOENGINE_ATLAS
#define XENOENGINE_ATLAS

#include "SDL.h"

#include <vector>
#include <algorithm>

#include "../grinliz/glrectangle.h"
#include "btexture.h"


class Atlas
{
public:
	Atlas();
	~Atlas();

	SDL_Surface* Generate( BTexture* array, int nTexture, int maxWidth );

	bool Map( const char* assetName, const grinliz::Vector2F& in, grinliz::Vector2F* out );

	BTexture btexture;

private:
	// gets copied
	struct Tex {
		Tex() : src( 0 )	{}
		Tex( BTexture* src, const grinliz::GLString& assetName ) { this->src = src; this->assetName = assetName; x = y = cx = cy = 0; }
		BTexture* src;
		int x, y, cx, cy;
		grinliz::GLString assetName;
	};
	static bool TexSorter( const Tex& i, const Tex& j ) { return i.src->PixelSize() > j.src->PixelSize(); }

	std::vector< SDL_Surface* > surfaces;
	std::vector< int > stack;
	std::vector< Tex > textures;
};


#endif // XENOENGINE_ATLAS