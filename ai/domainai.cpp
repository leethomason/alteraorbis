#include "domainai.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../script/corescript.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

using namespace grinliz;

DomainAI::DomainAI()
{

}

DomainAI::~DomainAI()
{

}

	void DomainAI::OnAdd(Chit* chit, bool initialize)
{
	super::OnAdd(chit, initialize);
	ticker.SetPeriod(2000 + chit->random.Rand(1000));
}


void DomainAI::OnRemove()
{
	super::OnRemove();
}


void DomainAI::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	this->EndSerialize(xs);
}


void DomainAI::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
}


int DomainAI::DoTick(U32 delta)
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if (!spatial) return ticker.Next();

	//GameItem* mainItem = parentChit->GetItem();
	Vector2I sector = spatial->GetSector();
	CoreScript* cs = CoreScript::GetCore(sector);

	if (ticker.Delta(delta)) {
		// Create workers, if needed.
		Rectangle2F b = ToWorld(InnerSectorBounds(sector));
		CChitArray arr;
		ItemNameFilter workerFilter(IStringConst::worker, IChitAccept::MOB);
		Context()->chitBag->QuerySpatialHash(&arr, b, 0, &workerFilter);
		if (arr.Empty()) {
	//		if (mainItem->wallet.gold >= WORKER_BOT_COST) {
	//			Transfer(&ReserveBank::Instance()->bank, &mainItem->wallet, WORKER_BOT_COST);
				Context()->chitBag->NewWorkerChit(cs->ParentChit()->GetSpatialComponent()->GetPosition(), parentChit->Team());
	//		}
		}
	}
	return ticker.Next();
}
