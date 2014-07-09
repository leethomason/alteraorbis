#include "batterycomponent.h"
#include "../xegame/chitbag.h"
#include "../game/worldmap.h"
#include "../game/circuitsim.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

using namespace grinliz;

void BatteryComponent::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	XARC_SER(xs, charge);
	ticker.Serialize(xs, "ticker");
	this->EndSerialize(xs);
}

void BatteryComponent::OnAdd(Chit* chit, bool initialize)
{
	super::OnAdd(chit, initialize);
}


void BatteryComponent::OnRemove()
{
	super::OnRemove();
}


int BatteryComponent::DoTick(U32 delta)
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if (!sc) return 0;

//	Vector2I pos = sc->GetPosition2DI();
//	Context()->worldMap->SetCircuit(pos.x, pos.y, CIRCUIT_BATTERY);
//	Context()->worldMap->SetCircuitRotation(pos.x, pos.y, LRint(sc->GetYRotation() / 90.0f));

	charge += ticker.Delta(delta);
	if (charge > 4) charge = 4;

	RenderComponent* rc = parentChit->GetRenderComponent();
	if (rc) {
		CStr<32> str;
		str.Format("%d/4", charge);
		rc->SetDecoText(str.c_str());
	}

	return ticker.Next();
}

