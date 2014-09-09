#ifndef ALTERA_ADVISER_INCLUDED
#define ALTERA_ADVISER_INCLUDED

#include "../gamui/gamui.h"

class CoreScript;

class Adviser
{
public:
	Adviser();

	void Attach(gamui::TextLabel *text, gamui::Image* image);
	void DoTick(int time, CoreScript* cs, int nWorkers, const int* buildCounts, int nBuildCounts);

private:
	gamui::TextLabel *text;
	gamui::Image* image;
};


#endif // ALTERA_ADVISER_INCLUDED
