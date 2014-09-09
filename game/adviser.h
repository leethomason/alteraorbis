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
	enum {
		DORMANT,
		GREETING,
		ADVISING
	};

	enum {
		DORMANT_TIME = 5000,
		GREETING_TIME = 8000
	};

	int state;
	int timer;
	float rotation;

	gamui::TextLabel *text;
	gamui::Image* image;
};


#endif // ALTERA_ADVISER_INCLUDED
