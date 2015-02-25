#ifndef TUTORIAL_WIDGET_INCLUDED
#define TUTORIAL_WIDGET_INCLUDED


#include "../gamui/gamui.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

class TutorialWidget : public gamui::IWidget
{
public:
	TutorialWidget();
	~TutorialWidget();

	void Init(gamui::Gamui* gamui, const gamui::ButtonLook& look, const gamui::LayoutCalculator& calculator );

	virtual float X() const				{ return background.X(); }
	virtual float Y() const				{ return background.Y(); }
	virtual float Width() const			{ return background.Width(); }
	virtual float Height() const		{ return background.Height(); }

	virtual void SetPos(float x, float y)		{}
	virtual void SetSize(float width, float h)	{}
	virtual bool Visible() const				{ return background.Visible(); }
	virtual void SetVisible(bool visible);

	void ItemTapped(const gamui::UIItem* item);

	void Add(const gamui::IWidget* item, const char* text);
	void ShowTutorial();

private:
	void DoLayout();

	int currentTutorial;

	gamui::LayoutCalculator calculator;
	gamui::Image		background;
	gamui::TextLabel	textLabel, progressLabel;
	gamui::PushButton	next, done;
	gamui::Image		adviser;
	gamui::Canvas		canvas;

	struct Tutorial {
		const gamui::IWidget* item;
		const char* text;
	};
	grinliz::CDynArray<Tutorial> tutorials;
};


#endif // TUTORIAL_WIDGET_INCLUDED
