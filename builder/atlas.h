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