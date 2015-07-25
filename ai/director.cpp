#include "director.h"
#include "../game/team.h"
#include "../xegame/chitcontext.h"
#include "../game/lumoschitbag.h"

using namespace grinliz;

void Director::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
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

	Vector2I playerSector = Context()->chitBag->GetHomeSector();
	if (playerSector.IsZero()) return ZERO;
	if (ToSector(herd) == playerSector) return ZERO;

	int playerTeam = Context()->chitBag->GetHomeTeam();
	GLASSERT(playerTeam);
	if (Team::Instance()->GetRelationship(herd->Team(), playerTeam) == ERelate::ENEMY) {
		GLOUTPUT(("SendHerd to player.\n"));
		return playerSector;
	}
	return ZERO;
}


void Director::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_ARRIVED) {
		Vector2I playerSector = Context()->chitBag->GetHomeSector();
		if (playerSector == ToSector(chit->Position())) {
			int playerTeam = Context()->chitBag->GetHomeTeam();
			GLASSERT(playerTeam);
			if (Team::Instance()->GetRelationship(chit->Team(), playerTeam) == ERelate::ENEMY) {
				GLOUTPUT(("Enemy arrived.\n"));
			}
		}
	}
	super::OnChitMsg(chit, msg);
}
