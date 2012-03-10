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

	bool Map( const char* assetName, const grinliz::Rectangle2F& in, grinliz::Rectangle2F* out );

	BTexture btexture;

private:
	// gets copied
	struct Tex {
		Tex() : src( 0 )	{}
		Tex( BTexture* src ) { this->src = src;  }
		BTexture* src;
	};
	static bool TexSorter( const Tex& i, const Tex& j ) { return i.src->PixelSize() > j.src->PixelSize(); }

	std::vector< SDL_Surface* > surfaces;
	std::vector< int > stack;
	std::vector< Tex > textures;
};


#endif // XENOENGINE_ATLAS