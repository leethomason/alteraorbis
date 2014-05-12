#ifndef ITEM_DESC_WIDGET_INCLUDED
#define ITEM_DESC_WIDGET_INCLUDED

#include "../gamui/gamui.h"
#include "../game/gamelimits.h"
#include "../grinliz/glstringutil.h"

class GameItem;
class ChitBag;

class ItemDescWidget : public gamui::IWidget
{
public:
	ItemDescWidget() : layout(1000, 1000), shortForm(false)		{}
	void Init( gamui::Gamui* gamui );

	virtual float X() const							{ return text.X(); }
	virtual float Y() const							{ return text.Y(); }
	virtual float Width() const						{ return text.Width(); }
	virtual float Height() const					{ return text.Height(); }

	void SetLayout( const gamui::LayoutCalculator& _layout )	{ layout = _layout; }
	virtual void SetPos( float x, float y );

	virtual void SetSize( float width, float h )	{}
	virtual bool Visible() const					{ return text.Visible(); }
	virtual void SetVisible( bool vis );

	void SetInfo( const GameItem* item, const GameItem* user, bool showPersonality, ChitBag* chitBag );
	void SetShortForm(bool s) { shortForm = s; }

private:
	bool shortForm;
	gamui::LayoutCalculator	layout;
	gamui::TextLabel		text;
	grinliz::CStr< 4096 >	textBuffer;
};


#endif // ITEM_DESC_WIDGET_INCLUDED

