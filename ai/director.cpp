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

	bool greater = (herd->GetItem()->MOB() == ISC::greater);
	bool denizen = (herd->GetItem()->MOB() == ISC::denizen);

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
