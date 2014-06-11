#ifndef XENO_TEXTURETYPE_INCLUDED
#define XENO_TEXTURETYPE_INCLUDED

enum TextureType 
{			// channels	bytes
	TEX_RGBA16,		// 4444		2
	TEX_RGB16,		// 565		2
	//TEX_ALPHA,		// 8		1	not supported in some GL contexts. As usual, it's not worth figuring out which.
	//									just burn texture memory and create and RGBA16 thats: 1,1,1,alpha
};

static bool TextureHasAlpha(TextureType t)			{ return t != TEX_RGB16; }
static int  TextureBytesPerPixel(TextureType t)		{ return 2; }


#endif // XENO_TEXTURETYPE_INCLUDED
