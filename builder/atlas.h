#ifndef XENOENGINE_ATLAS
#define XENOENGINE_ATLAS

#include "SDL.h"

#include <vector>
#include <algorithm>

#include "../engine/enginelimits.h"
#include "../grinliz/glrectangle.h"
#include "btexture.h"


// SHALLOW
class AtlasSubTex {
public:
	AtlasSubTex() : src( 0 )	{}
	AtlasSubTex( BTexture* src, const grinliz::GLString& assetName ) { this->src = src; this->assetName = assetName.c_str(); x = y = cx = cy = 0; }

	BTexture* src;
	int x, y, cx, cy, atlasCX, atlasCY;
	grinliz::CStr<EL_FILE_STRING_LEN> assetName;

	void Map( const grinliz::Vector2F& in, grinliz::Vector2F* out ) const;
};


class Atlas
{
public:
	Atlas();
	~Atlas();

	SDL_Surface* Generate( BTexture* array, int nTexture, int maxWidth );

	const AtlasSubTex* GetSubTex( const char* assetName ) const;

	BTexture btexture;

private:
	static bool TexSorter( const AtlasSubTex& i, const AtlasSubTex& j ) { return i.src->PixelSize() > j.src->PixelSize(); }

	std::vector< SDL_Surface* > surfaces;
	std::vector< int > stack;
	std::vector< AtlasSubTex > subTexArr;
};


#endif // XENOENGINE_ATLAS