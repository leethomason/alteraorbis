#ifndef FACEWIDGET_INCLUDED
#define FACEWIDGET_INCLUDED

#include "../gamui/gamui.h"

class GameItem;
class UIRenderer;

class FaceWidget : public gamui::IWidget
{
public:
	FaceWidget()	{}

	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook& ) = 0;
	void SetFace( UIRenderer* renderer, const GameItem* item );

	virtual float X() const							{ return button->X(); }
	virtual float Y() const							{ return button->Y(); }
	virtual float Width() const						{ return button->Width(); }
	virtual float Height() const					{ return button->Height(); }

	virtual void SetPos( float x, float y )			{ button->SetPos( x, y ); }
	virtual void SetSize( float w, float h )		{ button->SetSize( w, h ); }
	virtual bool Visible() const					{ return button->Visible(); }
	virtual void SetVisible( bool vis )				{ button->SetVisible( vis ); }

	const gamui::Button* GetButton() const			{ return button; }
	gamui::Button* GetButton()						{ return button; }

protected:
	gamui::Button* button;
};


class FaceToggleWidget : public FaceWidget
{
public:
	FaceToggleWidget() {
		button = &toggle;
	}

	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook& );
	gamui::ToggleButton* GetToggleButton()			{ return &toggle; }

private:
	gamui::ToggleButton toggle;
};


class FacePushWidget : public FaceWidget
{
public:
	FacePushWidget() {
		button = &push;
	}
	virtual void Init( gamui::Gamui* gamui, const gamui::ButtonLook& );

private:
	gamui::PushButton push;

};


#endif // FACEWIDGET_INCLUDED
