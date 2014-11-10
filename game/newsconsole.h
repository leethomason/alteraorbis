#ifndef NEWS_CONSOLE_INCLUDED
#define NEWS_CONSOLE_INCLUDED

#include "../gamui/gamui.h"
#include "../widget/consolewidget.h"

class LumosChitBag;
class CoreScript;

class NewsConsole
{
public:
	NewsConsole();
	~NewsConsole();

	void Init(gamui::Gamui* gamui2D, LumosChitBag* chitBag);

	ConsoleWidget consoleWidget;

	void DoTick(int time, CoreScript* homeCore);

private:
	void ProcessNewsToConsole(CoreScript* homeCore);
	
	LumosChitBag*	chitBag;
	int				currentNews;	// index of the last news item put in the console
};


#endif // NEWS_CONSOLE_INCLUDED
