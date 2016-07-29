#include "gamuifreetype.h"

#include <math.h>

using namespace gamui;

// FreeType:
// http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html

GamuiFreetypeBridge::GamuiFreetypeBridge()
{
	deltaHeight = 0;
	deltaLineSpacing = 0;
	deltaAscent = 0;
	deltaDescent = 0;
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


bool GamuiFreetypeBridge::Generate(int _height, uint8_t* pixels, int w, int h, bool correctHeight)
{
	fontHeight = _height + deltaHeight;
	textureWidth = w;
	textureHeight = h;

	static const int PAD = 1;

	// English. Sorry for non-international support.
	bool done = true;
	scale = 1;
	do {
		int x = 0;
		int y = 0;
		int maxHeight = 0;
		done = true;

		FT_Set_Pixel_Sizes(face, 0, fontHeight / scale);
		memset(pixels, 0, w*h);
		memset(glyphs, 0, sizeof(Glyph)*NUM_CODES);

		// Correct for bad heights.
		// Different fonts are different sizes, and it makes switching
		// annoying. Do a patch up here, from the notion that 11 point
		// Roboto returns an 8 pixel height for the test characters.
		if (scale == 1 && fontHeight > 4 && correctHeight)
		{
			int bestFontHeight = fontHeight;
			float bestError = 10.0f;
			int bestDelta = 100;

			for (int testHeight = fontHeight - 2; testHeight <= fontHeight + 2; ++testHeight) {
				FT_Set_Pixel_Sizes(face, 0, testHeight);
				static const int NTEST = 4;
				const char test[NTEST] = { 'H', 'I', 'W', 'A' };
				int height = 0;

				for (int i = 0; i < NTEST; ++i) {
					FT_UInt glyphIndex = FT_Get_Char_Index(face, 'H');
					FT_Load_Glyph(face, glyphIndex, 0);
					const FT_GlyphSlot& slot = face->glyph;
					height += slot->metrics.height;
				}
				height = (height/NTEST) >> 6;
				float error = fabsf(float(height) - float(fontHeight) * 8.0f / 11.0f);
				int delta = abs(height - fontHeight);
				if ((error < bestError) || (error == bestError && delta < bestDelta)) {
					bestError = error;
					bestFontHeight = testHeight;
					bestDelta = delta;
				}
			}
			FT_Set_Pixel_Sizes(face, 0, bestFontHeight);
		}

		for (int ccode = FIRST_CHAR_CODE; ccode < END_CHAR_CODE; ++ccode) {
			FT_UInt glyphIndex = FT_Get_Char_Index(face, ccode);
			FT_Error error = FT_Load_Glyph(face, glyphIndex, 0);
			GAMUIASSERT(error == 0);
			(void)error;

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
				done = false;
				scale *= 2;
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
	} while (!done);

	// The ascent and the descent for the font are often
	// wrong. They are set to the max/min of a particular
	// glyph, not the representative value. Sampling letters
	// seems to give better results.
	{
		FT_UInt glyphIndex = FT_Get_Char_Index(face, 'h');
		FT_Load_Glyph(face, glyphIndex, 0);
		const FT_GlyphSlot& slot = face->glyph;
		ascent = slot->metrics.horiBearingY >> 6;
	}
	{
		FT_UInt glyphIndex = FT_Get_Char_Index(face, 'q');
		FT_Load_Glyph(face, glyphIndex, 0);
		const FT_GlyphSlot& slot = face->glyph;
		descent = (abs(slot->metrics.height) - abs(slot->metrics.horiBearingY)) >> 6;
	}
	return 0;
}


void GamuiFreetypeBridge::GamuiGlyph(int c0, int cPrev,	// character, prev character
	gamui::IGamuiText::GlyphMetrics* metric)
{
	int idx = c0 - FIRST_CHAR_CODE;
	if (idx < 0 || idx >= NUM_CODES) {
		// Space.
		GlyphMetrics space;
		*metric = space;
		metric->advance = fontHeight * 3 / 10;
		return;
	}

	FT_UInt leftIndex = FT_Get_Char_Index(face, cPrev);
	FT_UInt rightIndex = FT_Get_Char_Index(face, c0);
	FT_Vector kerning = { 0, 0 };
	FT_Get_Kerning(face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning);

	const Glyph& glyph = glyphs[idx];

	metric->advance = (glyph.advance - (kerning.x >> 6)) * scale;
	metric->x = float(glyph.bitmapLeft * scale);
	metric->y = float(-glyph.bitmapTop * scale);
	metric->w = float(glyph.tw * scale);
	metric->h = float(glyph.th * scale);

	metric->tx0 = float(glyph.tx) / float(textureWidth);
	metric->ty0 = float(glyph.ty) / float(textureHeight);
	metric->tx1 = float(glyph.tx + glyph.tw) / float(textureWidth);
	metric->ty1 = float(glyph.ty + glyph.th) / float(textureHeight);
}


void GamuiFreetypeBridge::GamuiFont(gamui::IGamuiText::FontMetrics* metric)
{
	// http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
	/* This seems correct, but gives very un-latin values.
	metric->ascent = face->ascender >> 6;
	metric->descent = -(face->descender >> 6);
	metric->linespace = face->height >> 6;
	*/
	metric->ascent  = (ascent + deltaAscent) * scale;
	metric->descent = (descent + deltaDescent) * scale;

	metric->linespace = (ascent + descent + deltaLineSpacing) * scale;
}
