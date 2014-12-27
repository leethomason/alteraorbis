#ifndef FACEWIDGET_INCLUDED
#define FACEWIDGET_INCLUDED

#include "../gamui/gamui.h"
#include "barstack.h"
#include "hpbar.h"

class GameItem;
class UIRenderer;
class ItemComponent;
class AIComponent;

class FaceWidget : public gamui::IWidget
{
public:
	FaceWidget()	{}

	enum {
		HP_BAR = 0x01,		// also shield
		AMMO_BAR = 0x02,
		LEVEL_BAR = 0x08,
		MORALE_BAR = 0x10,

		FOOD_BAR   = 0x020,
		ENERGY_BAR = 0x040,
		FUN_BAR    = 0x080,

		BATTLE_BARS = HP_BAR | AMMO_BAR | LEVEL_BAR,
		NEED_BARS	= MORALE_BAR | FOOD_BAR | ENERGY_BAR | FUN_BAR,
		ALL_BARS	= NEED_BARS | LEVEL_BAR | AMMO_BAR | HP_BAR,

		SHOW_NAME	= 0x200,
		ALL			= 0xffff
	};

	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook&, int flags, int id ) = 0;
	void SetFace( UIRenderer* renderer, const GameItem* item );
	void SetMeta( ItemComponent* ic, AIComponent* ai );

	virtual float X() const							{ return GetButton()->X(); }
	virtual float Y() const							{ return GetButton()->Y(); }
	virtual float Width() const						{ return GetButton()->Width(); }
	virtual float Height() const					{ return GetButton()->Height(); }

	virtual void SetPos( float x, float y );
	// always sets the size of the button (ignores flags)
	virtual void SetSize( float w, float h );
	virtual bool Visible() const					{ return GetButton()->Visible(); }
	virtual void SetVisible( bool vis );

	virtual const gamui::Button* GetButton() const = 0;
	virtual gamui::Button* GetButton() = 0;

	int Flags() const { return flags; }
	void SetFlags(int f);

protected:
	void BaseInit( gamui::Gamui* gamui, const gamui::ButtonLook& look, int flags, int id );

	int					flags;
	int					id;
	gamui::TextLabel	upper;
	enum {
		BAR_HP,
		BAR_AMMO,
		BAR_LEVEL,
		BAR_MORALE,
		BAR_FOOD,
		BAR_ENERY,
		BAR_FUN,
		MAX_BARS
	};
	BarStackWidget	barStack;
};


class FaceToggleWidget : public FaceWidget
{
public:
	FaceToggleWidget() {
	}

	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook&, int flags, int id );

	virtual const gamui::Button* GetButton() const		{ return &toggle; }
	virtual gamui::Button* GetButton()					{ return &toggle; }
private:
	gamui::ToggleButton toggle;
};


class FacePushWidget : public FaceWidget
{
public:
	FacePushWidget() {
	}
	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook&, int flags, int id );

	virtual const gamui::Button* GetButton() const		{ return &push; }
	virtual gamui::Button* GetButton()					{ return &push; }
private:
	gamui::PushButton push;

};


#endif // FACEWIDGET_INCLUDED
