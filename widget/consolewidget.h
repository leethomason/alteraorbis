#ifndef ALTERA_CONSOLE_WIDGET
#define ALTERA_CONSOLE_WIDGET

#include "../gamui/gamui.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"

class ConsoleWidget : public gamui::IWidget
{
public:
	ConsoleWidget();
	void Init( gamui::Gamui* );

	virtual float X() const							{ return text.X(); }
	virtual float Y() const							{ return text.Y(); }
	virtual float Width() const						{ return text.Width(); }
	virtual float Height() const					{ return text.Height(); }
	virtual void SetPos( float x, float y )			{ text.SetPos( x, y ); }

	virtual void SetSize( float w, float h )		{}
	void SetBounds( float w, float h );
	virtual bool Visible() const					{ return text.Visible(); }
	virtual void SetVisible( bool vis )				{ text.SetVisible( vis ); }

	void Push( const grinliz::GLString &str );
	void DoTick( U32 delta );

private:
	enum { 
		NUM_LINES = 10,		// expediant hack
		AGE_TIME  = 5000,	// msec
	};
	gamui::TextLabel	text;
	grinliz::GLString	strBuffer;
	grinliz::CArray< grinliz::GLString, NUM_LINES >	lines;
	grinliz::CArray< int, NUM_LINES >				age;
};


#endif // ALTERA_CONSOLE_WIDGET


