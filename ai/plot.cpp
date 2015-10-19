#include "plot.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/chitcontext.h"
#include "../game/news.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
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
		IString text = StringPool::Intern("The Swarm is moving.");
		NewsEvent newsEvent(NewsEvent::PLOT_EVENT, ToWorld2F(SectorBounds(current).Center()), 0, 0, &text);
		context->chitBag->GetNewsHistory()->Add(newsEvent);
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


void EvilRisingPlot::Init(const ChitContext* _context, 
			  const grinliz::Vector2I& _destSector, 
			  const grinliz::IString& _critter,
			  int _critterTeam,
			  const grinliz::IString& _demiName,
			  bool _demiIsFemale)
{
	context			= _context;
	destSector		= _destSector;
	critter			= _critter;
	critterTeam		= _critterTeam;
	demiName		= _demiName;
	demiIsFemale	= _demiIsFemale;
	overTime.SetPeriod(MAX_PLOT_TIME);
	overTime.Reset();
	eventTime.SetPeriod(1000);
	eventTime.Reset();

	GLOUTPUT(("EvilRisingPlot::Init\n"));
}


void EvilRisingPlot::Serialize(XStream* xs)
{
	XarcOpen(xs, "EvilRisingPlot");
	XARC_SER(xs, stage);
	XARC_SER(xs, badGuyID);
	XARC_SER(xs, destSector);
	XARC_SER(xs, critter);
	XARC_SER(xs, critterTeam);
	XARC_SER(xs, demiName);
	XARC_SER(xs, demiIsFemale);
	overTime.Serialize(xs, "overTime");
	eventTime.Serialize(xs, "eventTime");
	XarcClose(xs);
}


grinliz::Vector2I EvilRisingPlot::ShouldSendHerd(Chit* chit)
{
	static const Vector2I ZERO = { 0, 0 };
	if (stage == SWARM_STAGE) {
		if ((chit->ID() == badGuyID) && (ToSector(chit->Position()) != destSector)) {
			return destSector;
		}
		if ((chit->GetItem()->IName() == critter) && (ToSector(chit->Position()) != destSector)) {
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
		&& eventTime.FractionRemaining() < 0.5f) 
	{
		return destSector;
	}
	static const Vector2I ZERO = { 0, 0 };
	return ZERO;
}
 

bool EvilRisingPlot::DoTick(U32 time)
{
	CoreScript* coreScript = CoreScript::GetCore(destSector);
	Chit* badGuy = context->chitBag->GetChit(badGuyID);
	CStr<128> str;

	const SectorData& sd = context->worldMap->GetSectorData(destSector);
	if (!sd.HasCore()) {
		return true;	// how did we get somewhere without a core??
	}

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
			// FIXME 0 is testing value!!!
			if (totalEvil >= 0) {
				badGuy = context->chitBag->NewBadGuy(ToWorld2I(coreScript->ParentChit()->Position()),
														   demiName,
														   ISC::humanFemale,
														   critterTeam,
														   LESSER_DEITY_LEVEL);
				badGuyID = badGuy->ID();

				str.Format("The Flaw %s has gained entry to the Cores. %s commands the %s.", 
						   demiName.safe_str(),
						   demiIsFemale ? "She" : "He",
						   critter.safe_str());

				IString text = StringPool::Intern(str.c_str());
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
		mapBounds.Set(1, 1, NUM_SECTORS - 2, NUM_SECTORS - 2);

		CArray<Vector2I, 10> sector;
		CArray<float, 10> score;
		CoreScript* playerCore = context->chitBag->GetHomeCore();

		for (Rectangle2IIterator it(bounds); !it.Done(); it.Next()) {
			if (mapBounds.Contains(it.Pos()) && it.Pos() != destSector) {
				CoreScript* targetCore = CoreScript::GetCore(it.Pos());
				if (   targetCore 
					&& Team::Instance()->GetRelationship(badGuy, targetCore->ParentChit()) == ERelate::ENEMY
					&& ((targetCore != playerCore) || (playerCore->NumTemples() > TEMPLES_REPELS_GREATER)))
				{
					sector.PushIfCap(it.Pos());
					score.PushIfCap(float(NUM_SECTORS_2) - (it.Pos() - mapBounds.Center()).LengthSquared());
				}
			}
		}
		if (sector.Size()) {
			int idx = badGuy->random.Select(score.Mem(), score.Size());
			destSector = sector[idx];

			str.Format("The Flaw %s and swarm of %s is moving.",
					   demiName.safe_str(),
					   critter.safe_str());
			IString text = StringPool::Intern(str.c_str());
			NewsEvent newsEvent(NewsEvent::PLOT_EVENT, ToWorld2F(SectorBounds(destSector).Center()), 0, 0, &text);
			context->chitBag->GetNewsHistory()->Add(newsEvent);
		}
		return false;
	}

	if (stage == GROWTH_STAGE && overTime.Delta(time)) {
		// Don't need a PLOT_END because no PLOT_START was sent.
		GLOUTPUT(("EvilRisingPlot out of time.\n"));
		return true;
	}
	if (stage != GROWTH_STAGE && !badGuy) {
		str.Format("The Flaw %s is deleted. The %s swarm is over.",
				   demiName.safe_str(),
				   critter.safe_str());
		IString text = StringPool::Intern(str.c_str());

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
