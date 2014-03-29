#ifndef START_GAME_WIDGET
#define START_GAME_WIDGET

#include "../gamui/gamui.h"

class StartGameWidget : public gamui::IWidget
{
public:
	StartGameWidget();
	void Init(gamui::Gamui* gamui, const gamui::ButtonLook& look, const gamui::LayoutCalculator& calculator );
	//void InitColors(const )	// decided to wait on colors: not hard to do, but need to track both
								// 'team' and 'teamColors' in NewBuilding

	~StartGameWidget();

	virtual float X() const				{ return background.X(); }
	virtual float Y() const				{ return background.Y(); }
	virtual float Width() const			{ return background.Width(); }
	virtual float Height() const		{ return background.Height(); }

	virtual void SetPos(float x, float y);
	virtual void SetSize(float width, float h);
	virtual bool Visible() const				{ return background.Visible(); }
	virtual void SetVisible(bool visible) ;

private:
	float				textHeight;

	gamui::LayoutCalculator calculator;
	gamui::Image		background;
	gamui::TextLabel	topLabel, bodyLabel, countLabel;
	//gamui::Image		primaryColor, secondaryColor;
	//gamui::PushButton	prevColor, nextColor;
	gamui::PushButton	prevDomain, nextDomain;
	gamui::PushButton	okay;
};


#endif // START_GAME_WIDGET
