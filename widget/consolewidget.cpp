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
	}
	background.Init(g, nullAtom, true);
}


void ConsoleWidget::SetBackground(RenderAtom atom)
{
	background.SetAtom(atom);
}


void ConsoleWidget::SetSize( float w, float h )
{
	nLines = LRint(h / gamui->GetTextHeight());
	nLines = Clamp(nLines, 1, (int)NUM_LINES);
	for (int i = 0; i < NUM_LINES; ++i) {
		lines[i].text.SetVisible(i < nLines);
		lines[i].button.SetVisible(i < nLines);
	}
	background.SetSize(w, h);
}


void ConsoleWidget::Scroll()
{
	RenderAtom a0, a1;

	for( int i=NUM_LINES-1; i>0; --i ) {
		lines[i].age = lines[i-1].age;
		lines[i].pos = lines[i-1].pos;

		const char* t = lines[i-1].text.GetText();
		lines[i].text.SetText(t);

		lines[i-1].button.GetDeco(&a0, &a1);
		lines[i].button.SetDeco(a0, a1);
	}
	RenderAtom nullAtom;
	lines[0].age = 0;
	lines[0].pos.Zero();
	lines[0].text.SetText("");
	lines[0].button.SetDeco(nullAtom, nullAtom);
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
	}
}


float ConsoleWidget::Height() const
{
	return nLines * gamui->GetTextHeight();
}


void ConsoleWidget::SetPos(float x, float y)
{
	float h = gamui->GetTextHeight();
	for (int i = 0; i < NUM_LINES; ++i) {
		lines[i].button.SetPos(x, y + (float)i*h);
		lines[i].button.SetSize(h, h);
		lines[i].text.SetPos(x + h, y + (float)i*h);
	}
	background.SetPos(x, y);
}


void ConsoleWidget::SetVisible(bool vis)
{
	for (int i = 0; i < NUM_LINES; ++i) {
		bool visible = vis && (i < nLines);
		lines[i].text.SetVisible(visible);
		lines[i].button.SetVisible(visible);
	}
	background.SetVisible(vis);
}


void ConsoleWidget::Push(const grinliz::GLString &str)
{
	Scroll();
	lines[0].text.SetText(str.safe_str());
	lines[0].age = 0;
}


void ConsoleWidget::Push(const grinliz::GLString &str, gamui::RenderAtom icon, const grinliz::Vector2F& pos)
{
	Scroll();
	lines[0].text.SetText(str.safe_str());
	lines[0].age = 0;
	lines[0].pos = pos;
	lines[0].button.SetDeco(icon, icon);
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
