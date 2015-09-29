#include "director.h"
#include "../game/team.h"
#include "../game/gameitem.h"
#include "../xegame/chitcontext.h"
#include "../game/lumoschitbag.h"
#include "../script/corescript.h"

using namespace grinliz;

void Director::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	XARC_SER(xs, attractLesser);
	XARC_SER(xs, attractGreater);
	attackTicker.Serialize(xs, "attackTicker");
	EndSerialize(xs);
}

void Director::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	Context()->chitBag->SetNamedChit(StringPool::Intern("Director"), parentChit);
	Context()->chitBag->AddListener(this);
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

	if (   plot == EPlot::SWARM 
		&& gameItem->IName() == plotCritter
		&& CoreScript::GetCore(plotCurrent)) 
	{
		return plotCurrent;
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
	if (plot != EPlot::NONE && plotTicker.Delta(delta)) {
		AdvancePlot();
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


void Director::Swarm(const IString& critter, const grinliz::Vector2I& start, const grinliz::Vector2I& end)
{
	plot = EPlot::SWARM;
	plotCritter = critter;
	plotStart = start;
	plotEnd = end;
	plotCurrent = start;
	plotTicker.SetPeriod(SWARM_TIME);
	plotTicker.Reset();

	NewsEvent newsEvent(NewsEvent::PLOT_SWARM_START, ToWorld2F(SectorBounds(plotStart).Center()), 0, 0);
	Context()->chitBag->GetNewsHistory()->Add(newsEvent);
}


void Director::AdvancePlot()
{
	GLASSERT(plot == EPlot::SWARM);
	if (plotCurrent != plotEnd) {
		if (abs(plotCurrent.x - plotEnd.x) > abs(plotCurrent.y - plotEnd.y)) {
			if (plotCurrent.x < plotEnd.x)
				plotCurrent.x++;
			else if (plotCurrent.x > plotEnd.x)
				plotCurrent.x--;
		}
		else {
			if (plotCurrent.y < plotEnd.y)
				plotCurrent.y++;
			else if (plotCurrent.y > plotEnd.y)
				plotCurrent.y--;
		}
		plotTicker.SetPeriod(plotCurrent == plotEnd ? SWARM_TIME * 2 : SWARM_TIME);
		plotTicker.Reset();
	}
	else {
		plot = EPlot::NONE;
	}
}
