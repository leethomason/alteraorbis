#include "plot.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/chitcontext.h"
#include "../game/news.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../script/corescript.h"

using namespace grinliz;

Plot* Plot::Factory(const char* n) 
{
	if (StrEqual(n, "null")) {
		return 0;
	}
	else if (StrEqual(n, "SwarmPlot")) {
		return new SwarmPlot();
	}
	GLASSERT(false);
	return 0;
}


void SwarmPlot::Init(const ChitContext* c, const IString& _critter, const Vector2I& _start, const Vector2I& _end)
{
	context = c;
	critter = _critter;
	start = _start;
	end = _end;
	current = start;
	ticker.SetPeriod(SWARM_TIME);
	ticker.Reset();

	IString text = StringPool::Intern("A Swarm is beginning.", true);
	NewsEvent newsEvent(NewsEvent::PLOT_START, ToWorld2F(SectorBounds(start).Center()), 0, 0, &text);
	context->chitBag->GetNewsHistory()->Add(newsEvent);
}

void SwarmPlot::Serialize(XStream* xs)
{
	XarcOpen(xs, "SwarmPlot");
	XARC_SER(xs, start);
	XARC_SER(xs, end);
	XARC_SER(xs, current);
	XARC_SER(xs, critter);
	ticker.Serialize(xs, "ticker");
	XarcClose(xs);
}


Vector2I SwarmPlot::ShouldSendHerd(Chit* chit)
{
	const GameItem* gameItem = chit->GetItem();
	if (gameItem->IName() == critter
		&& CoreScript::GetCore(current))
	{
		return current;
	}
	static const Vector2I ZERO = { 0, 0 };
	return ZERO;
}

bool SwarmPlot::DoTick(U32 time)
{
	if (ticker.Delta(time)) {
		return AdvancePlot();
	}
	return false;
}

bool SwarmPlot::AdvancePlot()
{
	if (current != end) {
		if (abs(current.x - end.x) > abs(current.y - end.y)) {
			if (current.x < end.x)
				current.x++;
			else if (current.x > end.x)
				current.x--;
		}
		else {
			if (current.y < end.y)
				current.y++;
			else if (current.y > end.y)
				current.y--;
		}
		ticker.SetPeriod(current == end ? SWARM_TIME * 2 : SWARM_TIME);
		ticker.Reset();
	}
	else {
		IString text = StringPool::Intern("The Swarm is over.");
		NewsEvent newsEvent(NewsEvent::PLOT_END, ToWorld2F(SectorBounds(end).Center()), 0, 0, &text);
		context->chitBag->GetNewsHistory()->Add(newsEvent);
		return true;
	}
	return false;
}


void GreatBattlePlot::Init(const ChitContext* _context, const grinliz::Vector2I& _dest)
{
	dest = _dest;
	ticker.SetPeriod(BATTLE_TIME);
	ticker.Reset();
	context = _context;

	IString text = StringPool::Intern("The Great Battle begins.");
	NewsEvent newsEvent(NewsEvent::PLOT_START, ToWorld2F(SectorBounds(dest).Center()), 0, 0, &text);
	context->chitBag->GetNewsHistory()->Add(newsEvent);
}


grinliz::Vector2I GreatBattlePlot::ShouldSendHerd(Chit* chit)
{
	if (chit->GetItem()->MOB() == ISC::greater) {
		return dest;
	}
	static const Vector2I ZERO = { 0, 0 };
	return ZERO;
}

void GreatBattlePlot::Serialize(XStream* xs) 
{
	XarcOpen(xs, "GreatBattlePlot");
	XARC_SER(xs, dest);
	ticker.Serialize(xs, "ticker");
	XarcClose(xs);
}

bool GreatBattlePlot::DoTick(U32 time)
{
	if (ticker.Delta(time)) {
		IString text = StringPool::Intern("The Great Battle is over.");
		NewsEvent newsEvent(NewsEvent::PLOT_END, ToWorld2F(SectorBounds(dest).Center()), 0, 0, &text);
		context->chitBag->GetNewsHistory()->Add(newsEvent);
		return true;
	}
	return false;
}
