#ifndef END_GAME_WIDGET
#define END_GAME_WIDGET

#include "../gamui/gamui.h"
#include "../script/corescript.h"

class SectorData;
class Engine;
class ChitBag;
class Chit;
class Scene;
class NewsHistory;

class EndGameWidget : public gamui::IWidget
{
public:
	EndGameWidget();
	void Init(gamui::Gamui* gamui, const gamui::ButtonLook& look, const gamui::LayoutCalculator& calculator );

	~EndGameWidget();

	const char* Name() const { return "EndGameWidget"; }

	virtual float X() const				{ return background.X(); }
	virtual float Y() const				{ return background.Y(); }
	virtual float Width() const			{ return background.Width(); }
	virtual float Height() const		{ return background.Height(); }

	virtual void SetPos(float x, float y);
	virtual void SetSize(float width, float h);
	virtual bool Visible() const				{ return background.Visible(); }
	virtual void SetVisible(bool visible) ;

	void ItemTapped(const gamui::UIItem* item);

	void SetData(NewsHistory* history, Scene* callback, grinliz::IString name, int itemID, const CoreAchievement& achieve);

private:
	void SetBodyText();

	float				textHeight;
	Engine*				engine;
	ChitBag*			chitBag;
	Scene*				iScene;
	NewsHistory*		history;
	int					itemID;

	CoreAchievement		achievement;
	grinliz::IString	coreName;

	gamui::LayoutCalculator calculator;
	gamui::Image		background;
	gamui::TextLabel	bodyLabel;
	gamui::PushButton	okay;
};


#endif // END_GAME_WIDGET
