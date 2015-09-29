#include "consolewidget.h"

using namespace grinliz;
using namespace gamui;

/*
	4 Newest
	3
	2
	1
	0 Oldest 
*/

void ConsoleWidget::Line::SetHighLightSize()
{
	float w = text.Width();
	float h = text.Height();
	highLight.SetSize(w, h);
}

ConsoleWidget::ConsoleWidget()
{
	nLines = NUM_LINES;
	ageTime = AGE_TIME;
}


void ConsoleWidget::Init( gamui::Gamui* g )
{
	gamui = g;
	gamui::RenderAtom nullAtom;
	for (int i = 0; i < NUM_LINES; ++i) {
		lines[i].age = 0;
		lines[i].text.Init(g);
		lines[i].button.Init(g, nullAtom, nullAtom, nullAtom, nullAtom, nullAtom, nullAtom);
		lines[i].pos.Zero();
		lines[i].highLight.Init(g, RenderAtom(), true);
	}
	background.Init(g, nullAtom, true);
}


void ConsoleWidget::SetBackground(RenderAtom atom)
{
	background.SetAtom(atom);
}


void ConsoleWidget::SetSize( float w, float h )
{
	nLines = LRint(h / gamui->TextHeightVirtual());
	nLines = Clamp(nLines, 1, (int)NUM_LINES);
	for (int i = 0; i < NUM_LINES; ++i) {
		lines[i].text.SetVisible(i < nLines);
		lines[i].button.SetVisible(i < nLines);
		lines[i].highLight.SetVisible(i < nLines);
		lines[i].SetHighLightSize();
	}
	background.SetSize(w, h);
}


void ConsoleWidget::Scroll()
{
	RenderAtom a0, a1;

	for (int i = NUM_LINES - 1; i > 0; --i) {
		lines[i].age = lines[i - 1].age;
		lines[i].pos = lines[i - 1].pos;

		const char* t = lines[i - 1].text.GetText();
		lines[i].text.SetText(t);

		lines[i - 1].button.GetDeco(&a0, &a1);
		lines[i].button.SetDeco(a0, a1);

		lines[i].highLight.SetAtom(lines[i - 1].highLight.GetRenderAtom());
		lines[i].SetHighLightSize();
	}
	RenderAtom nullAtom;
	lines[0].age = 0;
	lines[0].pos.Zero();
	lines[0].text.SetText("");
	lines[0].button.SetDeco(nullAtom, nullAtom);
	lines[0].highLight.SetAtom(RenderAtom());
}


void ConsoleWidget::DoTick( U32 delta )
{
	int last = -1;
	for( int i=0; i<NUM_LINES; ++i ) {
		lines[i].age += delta;
		const char* p = lines[i].text.GetText();
		if (p && *p) {
			last = i;
		}
	}
	if (last >= 0 && lines[last].age > ageTime) {
		RenderAtom nullAtom;
		lines[last].age = 0;
		lines[last].pos.Zero();
		lines[last].text.SetText("");
		lines[last].button.SetDeco(nullAtom, nullAtom);
		lines[last].highLight.SetAtom(RenderAtom());
	}
}


float ConsoleWidget::Height() const
{
	return nLines * gamui->TextHeightVirtual();
}


void ConsoleWidget::SetPos(float x, float y)
{
	float h = gamui->TextHeightVirtual();
	for (int i = 0; i < NUM_LINES; ++i) {
		lines[i].button.SetPos(x, y + (float)i*h);
		lines[i].button.SetSize(h, h);
		lines[i].text.SetPos(x + h, y + (float)i*h);
		lines[i].highLight.SetPos(x + h, y + i*h);
	}
	background.SetPos(x, y);
}


void ConsoleWidget::SetVisible(bool vis)
{
	for (int i = 0; i < NUM_LINES; ++i) {
		bool visible = vis && (i < nLines);
		lines[i].text.SetVisible(visible);
		lines[i].button.SetVisible(visible);
		lines[i].highLight.SetVisible(visible);
	}
	background.SetVisible(vis);
}


void ConsoleWidget::Push(const grinliz::GLString &str)
{
	Scroll();
	lines[0].text.SetText(str.safe_str());
	lines[0].age = 0;
	lines[0].highLight.SetAtom(RenderAtom());
}


void ConsoleWidget::Push(const grinliz::GLString &str, const gamui::RenderAtom& icon, const grinliz::Vector2F& pos, const gamui::RenderAtom& background)
{
	Scroll();
	lines[0].text.SetText(str.safe_str());
	lines[0].age = 0;
	lines[0].pos = pos;
	lines[0].button.SetDeco(icon, icon);
	lines[0].highLight.SetAtom(background);
	lines[0].SetHighLightSize();
}


bool ConsoleWidget::IsItem(const gamui::UIItem* item, grinliz::Vector2F* pos)
{
	for (int i = 0; i < NUM_LINES; ++i) {
		if (item == &lines[i].button) {
			*pos = lines[i].pos;
			return true;
		}
	}
	return false;
}
