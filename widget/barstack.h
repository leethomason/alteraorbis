#ifndef ALTERA_BARSTACK_WIDGET
#define ALTERA_BARSTACK_WIDGET

#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glvector.h"

class BarStackWidget : public gamui::IWidget
{
public:
	BarStackWidget();
	void Init(gamui::Gamui* g, int nBars);

	virtual float X() const							{ return barArr[0].X(); }
	virtual float Y() const							{ return barArr[0].Y(); }
	virtual float Width() const						{ return barArr[0].Width(); }
	virtual float Height() const					{ return float(nBars) * deltaY; }

	virtual void SetPos(float x, float y);
	virtual void SetSize(float w, float h);

	virtual bool Visible() const					{ return barArr[0].Visible(); }
	virtual void SetVisible(bool vis);

	void SetBarColor(int i, const gamui::RenderAtom& atom);
	void SetBarText(int i, const char*);
	void SetBarVisible(int i, bool vis);
	void SetBarRatio(int i, float ratio);

private:
	enum { MAX_BARS = 10};	// tacky; but UIItems don't support operator=. Still compiles, which is bugging me.

	float height, deltaY;
	int nBars;
	bool visibleArr[MAX_BARS];
	gamui::DigitalBar barArr[MAX_BARS];
};


#endif // ALTERA_BARSTACK_WIDGET


