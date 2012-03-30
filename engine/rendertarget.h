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

	void SetActive( bool active, Engine* engine );
	Texture* GetTexture() { return &texture; }

	int Width() const	{ return gpuMem.w; };
	int Height() const	{ return gpuMem.h; }

private:
	U32 frameBufferID, 
		depthBufferID,
		renderTextureID;

	GPUMem gpuMem;
	Texture texture;
	Screenport* screenport;
	Screenport* savedScreenport;
};


#endif // RENDERTARGET_INCLUDED