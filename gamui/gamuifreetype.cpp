#include "gamuifreetype.h"

#include <math.h>

using namespace gamui;

// FreeType:
// http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html

GamuiFreetypeBridge::GamuiFreetypeBridge()
{

}


GamuiFreetypeBridge::~GamuiFreetypeBridge()
{
	FT_Done_FreeType(library);
}


bool GamuiFreetypeBridge::Init(const char* facePath)
{
	FT_Error error = FT_Init_FreeType(&library);
	GAMUIASSERT(!error);
	if (error) return false;

	error = FT_New_Face(library, facePath, 0, &face);
	GAMUIASSERT(!error);
	if (error) return false;

	return true;
}

#define GAMUI_MAX(x, y) x > y ? x : y

void GamuiFreetypeBridge::Blit(uint8_t* target, int scanbytes, const FT_Bitmap& bitmap)
{
	for (int j = 0; j < bitmap.rows; ++j) {
		memcpy(target + scanbytes*j, bitmap.buffer + bitmap.pitch*j, bitmap.pitch);
	}
}


bool GamuiFreetypeBridge::Generate(int height, uint8_t* pixels, int w, int h)
{
	fontHeight = height;
	textureWidth = w;
	textureHeight = h;

	FT_Set_Pixel_Sizes(face, 0, height);

	int x = 0;
	int y = 0;
	int maxHeight = 0;
	memset(glyphs, 0, sizeof(Glyph)*NUM_CODES);

	static const int PAD = 1;

	// English. Sorry for non-international support.
	for (int ccode = FIRST_CHAR_CODE; ccode < END_CHAR_CODE; ++ccode) {
		FT_UInt glyphIndex = FT_Get_Char_Index(face, ccode);
		FT_Error error = FT_Load_Glyph(face, glyphIndex, 0);
		GAMUIASSERT(error == 0);
		// http://www.freetype.org/freetype2/docs/glyphs/glyphs-7.html
		error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		
		const FT_GlyphSlot& slot = face->glyph;
		const FT_Bitmap& bitmap = slot->bitmap;
		
		maxHeight = GAMUI_MAX(bitmap.rows, maxHeight);

		// Go to the next row?
		if (x + bitmap.width > w) {
			y += maxHeight + PAD;
			x = 0;
			maxHeight = 0;
		}
		// Out of space?
		if (y + maxHeight >= h) {
			GAMUIASSERT(false);
			break;
		}

		Blit(pixels + w*y + x, w, bitmap);

		int idx = ccode - FIRST_CHAR_CODE;
		glyphs[idx].tx = x;
		glyphs[idx].ty = y;
		glyphs[idx].tw = bitmap.width;
		glyphs[idx].th = bitmap.rows;

		glyphs[idx].advance = slot->advance.x >> 6;
		glyphs[idx].bitmapLeft = slot->bitmap_left;
		glyphs[idx].bitmapTop = slot->bitmap_top;

		x += bitmap.width + PAD;
	}
	return 0;
}


void GamuiFreetypeBridge::GamuiGlyph(int c0, int cPrev,	// character, prev character
	int height,
	gamui::IGamuiText::GlyphMetrics* metric)
{
	int idx = c0 - FIRST_CHAR_CODE;
	if (idx < 0 || idx >= NUM_CODES) {
		// Space.
		GlyphMetrics space;
		*metric = space;
		metric->advance = fontHeight / 2;
		return;
	}

	const Glyph& glyph = glyphs[idx];

	metric->advance = glyph.advance;
	metric->x = float(glyph.bitmapLeft);
	metric->y = float(-glyph.bitmapTop);
	metric->w = float(glyph.tw);
	metric->h = float(glyph.th);

	metric->tx0 = float(glyph.tx) / float(textureWidth);
	metric->ty0 = float(glyph.ty) / float(textureHeight);
	metric->tx1 = float(glyph.tx + glyph.tw) / float(textureWidth);
	metric->ty1 = float(glyph.ty + glyph.th) / float(textureHeight);
}


void GamuiFreetypeBridge::GamuiFont(int height, gamui::IGamuiText::FontMetrics* metric)
{
	// http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
	metric->ascent = face->ascender >> 6;
	metric->descent = -(face->descender >> 6);
	metric->linespace = face->height >> 6;
}
