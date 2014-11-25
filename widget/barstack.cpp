#include "barstack.h"
#include "../game/lumosgame.h"
#include "../game/layout.h"

using namespace gamui;

BarStackWidget::BarStackWidget()
{
	height = deltaY = 0;
}


BarStackWidget::~BarStackWidget()
{
	Deinit();
}


void BarStackWidget::Deinit()
{
	while (!barArr.Empty()) {
		delete barArr.Pop();
	}
	visibleArr.Clear();
}

void BarStackWidget::Init(gamui::Gamui* g, int nHPBars, int nBars)
{
	GLASSERT(height == 0);
	GLASSERT(deltaY == 0);

	for (int i = 0; i < nHPBars; ++i) {
		HPBar* hpBar = new HPBar();
		hpBar->Init(g);
		barArr.Push(hpBar);
	}

	RenderAtom gray  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	gray.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	blue.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;

	for (int i = 0; i < nBars; ++i) {
		DigitalBar* bar = new DigitalBar();
		bar->Init(g, blue, gray);
		barArr.Push(bar);
	}

	float rowHeight = LAYOUT_SIZE_Y * 0.5f;
	deltaY = rowHeight;
	height = deltaY * LAYOUT_BAR_HEIGHT_FRACTION;

	for (int i = 0; i < barArr.Size(); ++i) {
		visibleArr.Push(true);
		barArr[i]->SetSize(height*10.0f, height);
	}
}


void BarStackWidget::SetSize(float w, float /*h*/ )
{
	for (int i = 0; i < barArr.Size(); ++i) {
		barArr[i]->SetSize(w, height);
	}	
}

void BarStackWidget::SetPos(float x, float y)
{
	for (int i = 0; i < barArr.Size(); ++i) {
		barArr[i]->SetPos(x, y + float(i) * deltaY);
	}
}


void BarStackWidget::SetVisible(bool vis)
{
	for (int i = 0; i < barArr.Size(); ++i) {
		barArr[i]->SetVisible(vis && visibleArr[i]);
	}
}
