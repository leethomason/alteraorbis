#ifndef GAMUI_FREETYPE_INCLUDED
#define GAMUI_FREETYPE_INCLUDED

#include "gamui.h"

namespace gamui {

class GamuiFreetypeBridge : public IGamuiText
{
public:
	GamuiFreetypeBridge();
	~GamuiFreetypeBridge();

	bool Init(const char* facePath);
	uint8_t* Generate(int height, int* w, int* h);

	virtual void GamuiGlyph(int c0, int cPrev,	// character, prev character
		float height,
		gamui::IGamuiText::GlyphMetrics* metric);

private:
};

};	// namespace gamui

#endif // GAMUI_FREETYPE_INCLUDED
