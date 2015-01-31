#ifndef MAP_GRID_WIDGET_INCLUDED
#define MAP_GRID_WIDGET_INCLUDED

#include "../gamui/gamui.h"
#include "../game/gamelimits.h"

class CoreScript;
struct ChitContext;

class MapGridWidget : public gamui::IWidget
{
public:
	MapGridWidget();
	void Init( gamui::Gamui* gamui );
	void SetCompactMode(bool value);

	virtual float X() const							{ return textLabel.X(); }
	virtual float Y() const							{ return textLabel.Y(); }
	virtual float Width() const						{ return width; }
	virtual float Height() const					{ return height; }

	virtual void SetPos( float x, float y );
	virtual void SetSize(float width, float h);
	virtual bool Visible() const					{ return textLabel.Visible(); }
	virtual void SetVisible( bool vis );

	// One or both can be null.
	void Set(const ChitContext* context, CoreScript* cs, CoreScript* home);
	void Clear();

private:
	void DoLayout();

	bool				compact = false;
	float				width = 100.0f;
	float				height = 100.0f;
	gamui::TextLabel	textLabel;

	enum {
		FACE_IMAGE_0,
		FACE_IMAGE_1,
		FACE_IMAGE_2,

		MOB_COUNT_IMAGE_0,
		MOB_COUNT_IMAGE_1,
		MOB_COUNT_IMAGE_2,

		CIV_TECH_IMAGE,
		GOLD_IMAGE,
		DIPLOMACY_IMAGE,
		NUM_IMAGES,

		NUM_FACE_IMAGES = 3,
		FACE_IMAGE_START = FACE_IMAGE_0,
		FACE_IMAGE_END = FACE_IMAGE_2+1,
		MOB_COUNT_START = MOB_COUNT_IMAGE_0,
		MOB_COUNT_END = MOB_COUNT_IMAGE_2+1
	};

	gamui::Image		image[NUM_IMAGES];
};


#endif //  MAP_GRID_WIDGET_INCLUDED
