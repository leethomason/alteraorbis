#include "flagscript.h"

#include "../script/corescript.h"
#include "../game/team.h"
#include "../xegame/rendercomponent.h"
#include "../script/procedural.h"
#include "../xegame/istringconst.h"
#include "../game/gameitem.h"

using namespace grinliz;

FlagScript::FlagScript() : timer(511)
{
	sector.Zero();
	squadID = 0;
	colorsSet = false;
}


void FlagScript::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	XARC_SER(xs, sector);
	XARC_SER(xs, squadID);
	timer.Serialize(xs, "timer");
	EndSerialize(xs);
}


void FlagScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		timer.Randomize(parentChit->random.Rand());
	}
}


void FlagScript::Attach(const Vector2I& s, int id)
{
	sector = s;
	squadID = id;
}


int FlagScript::DoTick(U32 delta)
{
	if (!colorsSet) {
		CoreScript* cs = CoreScript::GetCore(sector);
		RenderComponent* rc = parentChit->GetRenderComponent();
		if (cs && cs->InUse() && rc) {
			ProcRenderInfo info;
			AssignProcedural(ISC::team,
							 false,
							 cs->ParentChit()->GetItem()->ID(),
							 cs->ParentChit()->Team(),
							 false,
							 0,
							 0,
							 &info);
			rc->SetProcedural(0, info);
			colorsSet = true;
		}
	}
	if (timer.Delta(delta)) {
		CStr<32> str;
		Vector2I pos2i = ToWorld2I(parentChit->Position());
		CoreScript* cs = CoreScript::GetCore(sector);
		RenderComponent* rc = parentChit->GetRenderComponent();
		if (cs) {
			const grinliz::CDynArray<grinliz::Vector2I>& wps = cs->GetWaypoints(squadID);
			int idx = wps.Find(pos2i);
			if (idx >= 0) {
				if (rc) {
					GLASSERT(squadID >= 0 && squadID < MAX_SQUADS);
					const char* NAME[MAX_SQUADS] = { "Alpha", "Beta", "Delta", "Omega" };
					str.Format("%s\n%s #%d", Team::Instance()->TeamName(cs->ParentChit()->Team()).safe_str(), NAME[squadID], idx+1);
				}
			}
		}
		if (rc) {
			rc->SetDecoText(str.safe_str());
		}
		if (str.empty()) {
			parentChit->QueueDelete();
		}
	}
	return timer.Next();
}




