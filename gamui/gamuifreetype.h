#ifndef GAMUI_FREETYPE_INCLUDED
#define GAMUI_FREETYPE_INCLUDED

#include "gamui.h"

#include "ft2build.h"		// grr. forces adding to header includes.
#include FT_FREETYPE_H

namespace gamui {

class GamuiFreetypeBridge : public IGamuiText
{
public:
	GamuiFreetypeBridge();
	~GamuiFreetypeBridge();

	bool Init(const char* facePath);
	bool Generate(int height, uint8_t* pixels, int w, int h);

	virtual void GamuiGlyph(int c0, int cPrev,	// character, prev character
		float height,
		gamui::IGamuiText::GlyphMetrics* metric);

	enum {
		FIRST_CHAR_CODE = 33,	// space (32) is special
		END_CHAR_CODE = 127,	// 126 is the last interesting character
		NUM_CODES = END_CHAR_CODE - FIRST_CHAR_CODE
	};

private:
	void Blit(uint8_t* target, int scanbytes, const FT_Bitmap& bitmap);

	struct Glyph {
		// location in texture:
		int tx, ty;
		int tw, th;

		// glyph info:
		int advance;		// in pixels
		int bitmapLeft;
		int bitmapTop;
	};

	int			fontHeight;
	int			textureWidth, textureHeight;
	FT_Library  library;
	FT_Face		face;

	Glyph glyphs[NUM_CODES];
};

};	// namespace gamui

#endif // GAMUI_FREETYPE_INCLUDED
