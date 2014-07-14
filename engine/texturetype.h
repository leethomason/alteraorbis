#ifndef XENO_TEXTURETYPE_INCLUDED
#define XENO_TEXTURETYPE_INCLUDED

enum TextureType 
{					// channels	bytes
	TEX_RGBA16,		// 4444		2
	TEX_RGB16,		// 565		2
	TEX_RGBA32,		// 8888		4
	TEX_RGB24,		// 888		3
	//TEX_ALPHA,		// 8		1	not supported in some GL contexts. As usual, it's not worth figuring out which.
	//									just burn texture memory and create and RGBA16 thats: 1,1,1,alpha
};

inline bool TextureHasAlpha(TextureType t)			{ return t == TEX_RGBA16 || t == TEX_RGBA32; }
inline int  TextureBytesPerPixel(TextureType t)		
{ 
	switch (t) {
		case TEX_RGBA16:	return 2;
		case TEX_RGB16:		return 2;
		case TEX_RGBA32:	return 4;
		case TEX_RGB24:		return 3;
		default: GLASSERT(false); break;
	}
	return 2;
}


inline const char* TextureString(TextureType t)
{
	static const char* names[4] = { "RGBA16", "RGB16", "RGBA32", "RGB24" };
	GLASSERT(int(t) >= 0 && int(t) < 4);
	return names[t];
}


#endif // XENO_TEXTURETYPE_INCLUDED
