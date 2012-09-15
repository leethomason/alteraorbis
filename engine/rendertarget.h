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

#ifndef RENDERTARGET_INCLUDED
#define RENDERTARGET_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "texture.h"
#include "screenport.h"


class Engine;


class RenderTarget
{
public:
	RenderTarget( int width, int height, bool depthBuffer );
	~RenderTarget();

	// Sets this active, initializes and resizes the screenport.
	void SetActive( bool active, Engine* engine );
	Texture* GetTexture() { return &texture; }
	Screenport* screenport;

	int Width() const	{ return texture.w; };
	int Height() const	{ return texture.h; }

private:
	U32 frameBufferID, 
		depthBufferID,
		renderTextureID;

	//GPUMem gpuMem;
	Texture texture;
	Screenport* savedScreenport;
};


#endif // RENDERTARGET_INCLUDED