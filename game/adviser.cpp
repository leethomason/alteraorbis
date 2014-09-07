#include "adviser.h"

Adviser::Adviser()
{
	text = 0;
	image = 0;


}

void Attach(gamui::TextLabel *text, gamui::Image* image);
void DoTick(int time, CoreScript* cs, int nWorkers);
