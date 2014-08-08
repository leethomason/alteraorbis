#ifndef ALTERA_CONSOLE_WIDGET
#define ALTERA_CONSOLE_WIDGET

#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glvector.h"

class ConsoleWidget : public gamui::IWidget
{
public:
	ConsoleWidget();
	void Init( gamui::Gamui* );

	virtual float X() const							{ return lines[0].text.X(); }
	virtual float Y() const							{ return lines[0].text.Y(); }
	virtual float Width() const						{ return lines[0].text.Width(); }
	virtual float Height() const;
	virtual void SetPos(float x, float y);

	virtual void SetSize( float w, float h )		{}
	void SetBounds( float w, float h );
	virtual bool Visible() const					{ return lines[0].text.Visible(); }
	virtual void SetVisible(bool vis);

	void Push( const grinliz::GLString &str );
	void Push( const grinliz::GLString &str, gamui::RenderAtom icon, const grinliz::Vector2F& pos );
	void DoTick( U32 delta );
	bool IsItem(const gamui::UIItem* item, grinliz::Vector2F* pos);

private:
	void Scroll();

	struct Line {
		Line() { age = 0; pos.Zero(); }

		gamui::TextLabel	text;
		int					age;
		gamui::PushButton	button;
		grinliz::Vector2F	pos;
	};

	enum { 
		NUM_LINES = 10,			// expediant hack
		AGE_TIME  = 40*1000,	// msec
	};
	gamui::Gamui*		gamui;
	int					nLines;
	Line				lines[NUM_LINES];
};


#endif // ALTERA_CONSOLE_WIDGET


