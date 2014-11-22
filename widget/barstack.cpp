#include "barstack.h"
#include "../game/lumosgame.h"
#include "../game/layout.h"

using namespace gamui;

static const float HEIGHT_FRACTION = 0.75f;
static const float BAR_ALPHA = 0.80f;

BarStackWidget::BarStackWidget()
{
	height = deltaY = 0;
	nBars = 0;
}


void BarStackWidget::Init(gamui::Gamui* g, int _nBars)
{
	GLASSERT(height == 0);
	GLASSERT(deltaY == 0);
	GLASSERT(_nBars <= MAX_BARS);
	nBars = _nBars;

	float rowHeight = LAYOUT_SIZE_Y * 0.5f;
	deltaY = rowHeight;
	height = deltaY * HEIGHT_FRACTION;

	RenderAtom gray  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	gray.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	blue.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;

	for (int i = 0; i < nBars; ++i) {
		barArr[i].Init(g, blue, gray);
		visibleArr[i] = true;
		barArr[i].SetSize(height*10.0f, height);
	}
}


void BarStackWidget::SetSize(float w, float /*h*/ )
{
	for (int i = 0; i < nBars; ++i) {
		barArr[i].SetSize(w, height);
	}	
}

void BarStackWidget::SetPos(float x, float y)
{
	for (int i = 0; i < nBars; ++i) {
		barArr[i].SetPos(x, y + float(i) * deltaY);
	}
}


void BarStackWidget::SetVisible(bool vis)
{
	for (int i = 0; i < nBars; ++i) {
		barArr[i].SetVisible(vis && visibleArr[i]);
	}
}
