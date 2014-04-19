#ifndef XENO_TEXTURETYPE_INCLUDED
#define XENO_TEXTURETYPE_INCLUDED

enum TextureType 
{			// channels	bytes
	TEX_RGBA16,		// 4444		2
	TEX_RGB16,		// 565		2
	TEX_ALPHA,		// 8		1
};

static bool TextureHasAlpha(TextureType t)			{ return t != TEX_RGB16; }
static int  TextureBytesPerPixel(TextureType t)		{ return t == TEX_ALPHA ? 1 : 2; }


#endif // XENO_TEXTURETYPE_INCLUDED
