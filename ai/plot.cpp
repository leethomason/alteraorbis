#include "plot.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/chitcontext.h"
#include "../game/news.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../script/corescript.h"
#include "../game/worldmap.h"

using namespace grinliz;

Plot* Plot::Factory(const char* n) 
{
	if (StrEqual(n, "null")) {
		return 0;
	}
	else if (StrEqual(n, "SwarmPlot")) {
		return new SwarmPlot();
	}
	else if (StrEqual(n, "GreatBattlePlot")) {
		return new GreatBattlePlot();
	}
	else if (StrEqual(n, "EvilRisingPlot")) {
		return new EvilRisingPlot();
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


void EvilRisingPlot::Init(const ChitContext* _context, const grinliz::Vector2I& _dest)
{
	context = _context;
	destSector = _dest;
	overTime.SetPeriod(MAX_PLOT_TIME);
	overTime.Reset();
	eventTime.SetPeriod(1000);
	eventTime.Reset();

	GLOUTPUT(("EvilRisingPlot::Init\n"));
}


grinliz::Vector2I EvilRisingPlot::ShouldSendHerd(Chit* chit)
{
	static const Vector2I ZERO = { 0, 0 };
	if (stage == SWARM_STAGE) {
		if (chit->ID() == badGuyID && ToSector(chit->Position()) != destSector) {
			return destSector;
		}
		if (chit->GetItem()->IName() == ISC::mantis && ToSector(chit->Position()) != destSector) {
			return destSector;
		}
	}
	return ZERO;
}


grinliz::Vector2I EvilRisingPlot::PrioritySendHerd(Chit* chit)
{
	if (chit->ID() == badGuyID 
		&& stage == SWARM_STAGE 
		&& ToSector(chit->Position()) != destSector
		&& eventTime.Next() < eventTime.Period() / 2) 
	{
		return destSector;
	}
	static const Vector2I ZERO = { 0, 0 };
	return ZERO;
}


void EvilRisingPlot::Serialize(XStream* xs)
{
	XarcOpen(xs, "EvilRisingPlot");
	XARC_SER(xs, stage);
	XARC_SER(xs, badGuyID);
	XARC_SER(xs, destSector);
	overTime.Serialize(xs, "overTime");
	eventTime.Serialize(xs, "eventTime");
	XarcClose(xs);
}


bool EvilRisingPlot::DoTick(U32 time)
{
	CoreScript* coreScript = CoreScript::GetCore(destSector);
	if (!coreScript) {
		return true;	// bad start?
	}
	Chit* badGuy = context->chitBag->GetChit(badGuyID);

	if (stage == GROWTH_STAGE) {
		if (!coreScript->InUse() && eventTime.Delta(time)) {
			// Check for "enough evil" to move to 2nd stage.
			Rectangle2I bounds = InnerSectorBounds(destSector);
			bounds.Outset(-2);
			int totalEvil = 0;
			for (Rectangle2IIterator it(bounds); !it.Done(); it.Next()) {
				const WorldGrid& wg = context->worldMap->GetWorldGrid(it.Pos());
				if (wg.Plant() && (wg.Plant() - 1 == EVIL_PLANT)) {
					totalEvil += wg.PlantStage() + 1;
				}
			}
			if (totalEvil >= 0) { // # needs tuning
				badGuy = context->chitBag->NewBadGuy(ToWorld2I(coreScript->ParentChit()->Position()),
														   "Vyllis",
														   ISC::humanFemale,
														   TEAM_GREEN_MANTIS,
														   LESSER_DEITY_LEVEL);
				badGuyID = badGuy->ID();

				IString text = StringPool::Intern("Lesser deity Vyllis has gained entry to the Cores.");
				NewsEvent newsEvent(NewsEvent::PLOT_START, ToWorld2F(SectorBounds(destSector).Center()), badGuy->GetItemID(), 0, &text);
				context->chitBag->GetNewsHistory()->Add(newsEvent);
				stage = SWARM_STAGE;
				eventTime.SetPeriod(SWARM_TIME);
				eventTime.Reset();
			}
		}
	}
	else if (stage == SWARM_STAGE && eventTime.Delta(time)) {
		static const int RAD = 3;
		Rectangle2I bounds, mapBounds;
		bounds.min = bounds.max = destSector;
		bounds.Outset(2);
		mapBounds.Set(1, 1, NUM_SECTORS - 3, NUM_SECTORS - 3);

		CArray<Vector2I, 10> sector;
		CArray<float, 10> score;

		for (Rectangle2IIterator it(bounds); !it.Done(); it.Next()) {
			if (mapBounds.Contains(it.Pos()) && it.Pos() != destSector) {
				CoreScript* targetCore = CoreScript::GetCore(it.Pos());
				if (targetCore && Team::Instance()->GetRelationship(badGuy, targetCore->ParentChit()) == ERelate::ENEMY) {
					sector.PushIfCap(it.Pos());
					score.PushIfCap(float(NUM_SECTORS_2) - (it.Pos() - mapBounds.Center()).LengthSquared());
				}
			}
		}
		if (sector.Size()) {
			int idx = badGuy->random.Select(score.Mem(), score.Size());
			destSector = sector[idx];
			IString text = StringPool::Intern("The reighn of Vyllis continues.");
			NewsEvent newsEvent(NewsEvent::PLOT_EVENT, ToWorld2F(SectorBounds(destSector).Center()), 0, 0, &text);
			context->chitBag->GetNewsHistory()->Add(newsEvent);
		}
		return false;
	}

	if (stage == GROWTH_STAGE && overTime.Delta(time)) {
		// Don't need a PLOT_END because no PLOT_START was sent.
		return true;
	}
	if (stage != GROWTH_STAGE && !badGuy) {
		IString text = StringPool::Intern("Vyllis is deleted and the Emerald Swarm is defeated.");
		NewsEvent newsEvent(NewsEvent::PLOT_END, ToWorld2F(SectorBounds(destSector).Center()), 0, 0, &text);
		context->chitBag->GetNewsHistory()->Add(newsEvent);
		return true;
	}
	return false;
}


bool EvilRisingPlot::SectorIsEvil(const grinliz::Vector2I& sector)
{
	if (stage == GROWTH_STAGE) {
		CoreScript* coreScript = CoreScript::GetCore(destSector);
		return coreScript && !coreScript->InUse() && (destSector == sector);
	} 
	else {
		Chit* badGuy = context->chitBag->GetChit(badGuyID);
		return badGuy && (ToSector(badGuy->Position()) == sector);
	}
	return false;
}
