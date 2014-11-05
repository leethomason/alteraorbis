#ifndef MONEY_WIDGET
#define MONEY_WIDGET

#include "../gamui/gamui.h"
#include "../game/gamelimits.h"

class Wallet;

class MoneyWidget : public gamui::IWidget
{
public:
	MoneyWidget();
	void Init( gamui::Gamui* gamui );

	virtual float X() const							{ return textLabel[0].X(); }
	virtual float Y() const							{ return textLabel[0].Y(); }
	virtual float Width() const						{ return image[COUNT-1].X() + image[COUNT-1].Width() - image[0].X(); }
	virtual float Height() const					{ return image[0].Height(); }

	virtual void SetPos( float x, float y );
	virtual void SetSize( float width, float h )	{}
	virtual bool Visible() const					{ return image[0].Visible(); }
	virtual void SetVisible( bool vis );

	void Set( const Wallet& wallet );

private:
	enum { COUNT = NUM_CRYSTAL_TYPES+1 };
	gamui::TextLabel	textLabel[COUNT];
	gamui::Image		image[COUNT];
};


#endif //  MONEY_WIDGET
