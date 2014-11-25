#ifndef ALTERA_BARSTACK_WIDGET
#define ALTERA_BARSTACK_WIDGET

#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glvector.h"
#include "hpbar.h"

class BarStackWidget : public gamui::IWidget
{
public:
	BarStackWidget();
	~BarStackWidget();

	void Init(gamui::Gamui* g, int nHPBars, int nBars);
	void Deinit();

	virtual float X() const							{ return barArr[0]->X(); }
	virtual float Y() const							{ return barArr[0]->Y(); }
	virtual float Width() const						{ return barArr[0]->Width(); }
	virtual float Height() const					{ return float(barArr.Size()) * deltaY; }

	virtual void SetPos(float x, float y);
	virtual void SetSize(float w, float h);

	virtual bool Visible() const					{ return barArr[0]->Visible(); }
	virtual void SetVisible(bool vis);

	// Helper functions just obscurred things:
	grinliz::CDynArray<gamui::DigitalBar*> barArr;

private:

	float height, deltaY;
	grinliz::CDynArray<bool> visibleArr;
};


#endif // ALTERA_BARSTACK_WIDGET


