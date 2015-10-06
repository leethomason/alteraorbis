#include "director.h"
#include "plot.h"
#include "../game/team.h"
#include "../game/gameitem.h"
#include "../xegame/chitcontext.h"
#include "../game/lumoschitbag.h"
#include "../script/corescript.h"

using namespace grinliz;

Director::Director()
{
	plotTicker.SetPeriod(AGE_IN_MSEC * 4 / 7);
	plotTicker.Reset();
}

Director::~Director()
{
	delete plot;
}


void Director::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	XARC_SER(xs, attractLesser);
	XARC_SER(xs, attractGreater);
	attackTicker.Serialize(xs, "attackTicker");
	plotTicker.Serialize(xs, "plotTicker");
	if (xs->Saving()) {
		if (plot) {
			plot->Serialize(xs);
		}
		else {
			XarcOpen(xs, "null");
			XarcClose(xs);
		}
	}
	else {
		const char* peek = xs->Loading()->PeekElement();
		plot = Plot::Factory(peek);
		if (plot) {
			plot->Serialize(xs);
		}
		else {
			XarcOpen(xs, "null");
			XarcClose(xs);
		}
	}
	EndSerialize(xs);
}

void Director::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	Context()->chitBag->SetNamedChit(StringPool::Intern("Director"), parentChit);
	Context()->chitBag->AddListener(this);
	if (plot) {
		plot->SetContext(Context());
	}
}

void Director::OnRemove()
{
	super::OnRemove();
}


Vector2I Director::ShouldSendHerd(Chit* herd)
{
	static const Vector2I ZERO = { 0, 0 };
	GLASSERT(herd->GetItem());
	if (!herd->GetItem()) return ZERO;
	const GameItem* gameItem = herd->GetItem();

	Vector2I plotHerd = plot ? plot->ShouldSendHerd(herd) : ZERO;
	if (!plotHerd.IsZero()) {
		return plotHerd;
	}

	bool greater = (gameItem->MOB() == ISC::greater);
	bool denizen = (gameItem->MOB() == ISC::denizen);

	if (greater && !attractGreater) 
		return ZERO;
	if (!greater && !attractLesser) 
		return ZERO;

	Vector2I playerSector = Context()->chitBag->GetHomeSector();
	CoreScript* playerCore = Context()->chitBag->GetHomeCore();
	if (playerSector.IsZero() || !playerCore) 
		return ZERO;
	if (ToSector(herd->Position()) == playerSector) 
		return ZERO;

	int nTemples = playerCore->NumTemples();
	if (nTemples == 0 && denizen)
		return ZERO;	// denizens can be fairly well armed.

	int playerTeam = Context()->chitBag->GetHomeTeam();
	GLASSERT(playerTeam);
	if (Team::Instance()->GetRelationship(herd->Team(), playerTeam) == ERelate::ENEMY) {
		GLOUTPUT(("SendHerd to player.\n"));

		if (greater)
			attractGreater = false;
		else
			attractLesser = false;

		return playerSector;
	}
	return ZERO;
}


void Director::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	Vector2I playerSector = Context()->chitBag->GetHomeSector();

#if 0
	if (msg.ID() == ChitMsg::CHIT_ARRIVED) {
		if (playerSector == ToSector(chit->Position())) {
			int playerTeam = Context()->chitBag->GetHomeTeam();
			GLASSERT(playerTeam);
			if (Team::Instance()->GetRelationship(chit->Team(), playerTeam) == ERelate::ENEMY) {
				GLOUTPUT(("Enemy arrived.\n"));
			}
		}
	}
	else 
#endif
	if (msg.ID() == ChitMsg::CHIT_DESTROYED) {
		if (playerSector == ToSector(chit->Position())) {
			attackTicker.Reset();
			//GLOUTPUT(("Attack ticker reset.\n"));
		}
	}
	super::OnChitMsg(chit, msg);
}


int Director::DoTick(U32 delta)
{
	CoreScript* coreScript = Context()->chitBag->GetHomeCore();
	Vector2I playerSector = Context()->chitBag->GetHomeSector();

	// Plot motion:
	if (plot) {
		bool done = plot->DoTick(delta);
		if (done) {
			delete plot; plot = 0;
		}
	}
	if (!plot && plotTicker.Delta(delta)) {
		StartRandomPlot();
		plotTicker.Reset();
	}

	// Basic player ticker motion:
	if (playerSector.IsZero()) {
		attackTicker.SetPeriod(5 * 1000 * 60);
		attackTicker.Reset();
		return VERY_LONG_TICK;
	}
	GLASSERT(coreScript);
	int nTemples = coreScript->NumTemples();
	int minutes = 5 - nTemples;
	minutes = Clamp(minutes, 1, 5);
	attackTicker.SetPeriod(minutes * 1000 * 60);

	if (attackTicker.Delta(delta)) {
		attractLesser = true;
		attractGreater = nTemples > 2;
	}
	return VERY_LONG_TICK;
}


void Director::StartRandomPlot()
{
	GLASSERT(!plot);
	if (plot) return;

	CoreScript* coreScript = Context()->chitBag->GetHomeCore();
	Vector2I playerSector = Context()->chitBag->GetHomeSector();
	int nTemples = coreScript ? coreScript->NumTemples() : 0;
	const Census& census = Context()->chitBag->census;
	
	int lesser = 0, greater = 0, denizen = 0;
	census.NumByType(&lesser, &greater, &denizen);

	enum {
		SWARM,
		GREAT_BATTLE,
		NPLOTS
	};
	float odds[NPLOTS] = { 0 };

	odds[SWARM] = 1.0;
	if (greater == TYPICAL_GREATER) {
		odds[GREAT_BATTLE] = 1.0;
	}
	int plotIndex = parentChit->random.Select(odds, NPLOTS);
	if (odds[plotIndex] == 0) return;	// nothing to select

	Vector2I start = { parentChit->random.Rand(NUM_SECTORS), parentChit->random.Rand(NUM_SECTORS) };
	Vector2I end   = { parentChit->random.Rand(NUM_SECTORS), parentChit->random.Rand(NUM_SECTORS) };

	bool target = coreScript && parentChit->random.Bit() && (nTemples >= MAX_HUMAN_TEMPLES - 1);

	if (plotIndex == SWARM) {
		int green = Context()->chitBag->census.NumCoresOfTeam(TEAM_GREEN_MANTIS);
		int red = Context()->chitBag->census.NumCoresOfTeam(TEAM_RED_MANTIS);
		IString critter = red > green ? ISC::redMantis : ISC::mantis;

		if (target) {
			end = playerSector;
		}
		Swarm(critter, start, end);
	}
	else if (plotIndex == GREAT_BATTLE) {
		if (target) {
			start = playerSector;
		}
		GreatBattle(start);
	}
}



void Director::Swarm(const IString& critter, const grinliz::Vector2I& start, const grinliz::Vector2I& end)
{
	GLASSERT(!plot);
	if (plot) return;

	SwarmPlot* swarmPlot = new SwarmPlot();
	swarmPlot->Init(Context(), critter, start, end);
	plot = swarmPlot;
}


void Director::GreatBattle(const grinliz::Vector2I& pos) {
	GLASSERT(!plot);
	if (plot) return;

	CoreScript* cs = CoreScript::GetCore(pos);
	if (!cs) return;

	GreatBattlePlot* greatBattlePlot = new GreatBattlePlot();
	greatBattlePlot->Init(Context(), pos);
	plot = greatBattlePlot;
}
