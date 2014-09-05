#include "barstack.h"
#include "../game/lumosgame.h"

using namespace gamui;

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
	height = g->GetTextHeight() * 0.9f;
	deltaY = g->GetTextHeight();

//	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	
	RenderAtom gray  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	for (int i = 0; i < nBars; ++i) {
		barArr[i].Init(g, 2, blue, gray);
		visibleArr[i] = true;
		barArr[i].SetSize(height*10.0f, height);
	}
}


void BarStackWidget::SetSize(float w, float _h)
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


void BarStackWidget::SetBarVisible(int i, bool vis)
{
	visibleArr[i] = vis;
	barArr[i].SetVisible(vis && visibleArr[i]);
}


void BarStackWidget::SetBarRatio(int i, float ratio)
{
	barArr[i].SetRange(ratio);
}


void BarStackWidget::SetBarColor(int i, const gamui::RenderAtom& atom)
{
	barArr[i].SetLowerAtom(atom);
}


void BarStackWidget::SetBarText(int i, const char* str)
{
	barArr[i].SetText(str);
}

