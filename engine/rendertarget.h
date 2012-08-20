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