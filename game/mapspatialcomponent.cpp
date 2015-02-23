/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mapspatialcomponent.h"
#include "worldmap.h"
#include "lumoschitbag.h"
#include "../engine/serialize.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../script/evalbuildingscript.h"
#include "../game/circuitsim.h"
#include "../script/procedural.h"
#include "../xegame/rendercomponent.h"
#include "../script/corescript.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent() : SpatialComponent()
{
	nextBuilding = 0;
	size = 1;
	blocks = false;
	hasPorch = 0;
	hasCircuit = 0;
	glow = 1;
	glowTarget = 1;
	bounds.Zero();
}


void MapSpatialComponent::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_POS_CHANGE) {
		SyncWithSpatial();
	}
}


void MapSpatialComponent::SyncWithSpatial()
{
	if (!parentChit) return;
	Vector3F pos = parentChit->Position();
	if (pos.IsZero()) {
		GLASSERT(bounds.min.IsZero());
		return;
	}

	Rectangle2I oldBounds = bounds;

	// Position is in the center!
	if (size == 1) {
		bounds.min = ToWorld2I(pos);
	}
	else if (size == 2) {
		bounds.min = ToWorld2I(pos);
		bounds.min.x -= 1;
		bounds.min.y -= 1;
	}
	else {
		GLASSERT(0);
	}

	bounds.max.x = bounds.min.x + size - 1;
	bounds.max.y = bounds.min.y + size - 1;

	if (oldBounds != bounds) {
		// We have a new position, update in the hash tables:
		Context()->chitBag->RemoveFromBuildingHash(this, oldBounds.min.x, oldBounds.min.y);
		Context()->chitBag->AddToBuildingHash(this, bounds.min.x, bounds.min.y);
	}
	// And the pather.
	if (!oldBounds.min.IsZero()) {
		Context()->worldMap->UpdateBlock(oldBounds);
	}
	Context()->worldMap->UpdateBlock(bounds);

	// Compute a new porch type:
	EvalBuildingScript* ebs = 0;
	if (hasPorch) {
		ebs = (EvalBuildingScript*)parentChit->GetComponent("EvalBuildingScript");
		const GameItem* item = parentChit->GetItem();
		hasPorch = WorldGrid::BASE_PORCH;
		if (ebs && item) {
			double eval = ebs->EvalIndustrial(false);	// sets "Reachable". Must be called first.
			if (ebs->Reachable()) {
				double consumes = item->GetBuildingIndustrial();
				if (consumes) {
					double dot = eval * consumes;
					int q = int((1.0 + dot) * 2.0 + 0.5);
					// q=0, no porch. q=1 default.
					hasPorch = WorldGrid::BASE_PORCH + 1 + q;
					GLASSERT(hasPorch > 1 && hasPorch < WorldGrid::NUM_PORCH);
				}
			}
			else {
				hasPorch = WorldGrid::PORCH_UNREACHABLE;
			}
		}

	}
	CoreScript* cs = CoreScript::GetCore(ToSector(ToWorld2I(pos)));
	if (cs && cs->InUse()) {
		glowTarget = 1;
		if (ebs) {
			glowTarget = ebs->Reachable() ? 1.0f : 0.0f;
		}
	}
	else {
		glowTarget = 0;
	}

	// And the porches / circuits: (rotation doesn't change bounds);
	Rectangle2I oldOutset = oldBounds, outset = bounds;
	oldOutset.Outset(1);
	outset.Outset(1);

	if (!oldBounds.min.IsZero()) {
		UpdateGridLayer(Context()->worldMap, Context()->chitBag, Context()->circuitSim, oldOutset);
	}
	UpdateGridLayer(Context()->worldMap, Context()->chitBag, Context()->circuitSim, outset);
}


int MapSpatialComponent::DoTick(U32 delta)
{
	if (glow != glowTarget) {
		glow = TravelTo(0.7f, delta, glow, glowTarget);
	}
	if (parentChit->GetItem() && parentChit->GetItem()->IName() == ISC::core) {
		glow = glowTarget = 1;
	}

	RenderComponent* rc = parentChit->GetRenderComponent();
	if (rc) {
		const GameItem* gameItem = parentChit->GetItem();
		if (gameItem && gameItem->keyValues.GetIString(ISC::procedural) == ISC::team) {

			int team = parentChit->Team();
			ProcRenderInfo info;
			AssignProcedural(parentChit->GetItem(), &info);
			info.color.m41 *= glow;
			info.color.m42 *= glow;
			info.color.m43 *= glow;
			rc->SetProcedural(0, info);
			rc->SetSaturation(0.5f + 0.5f*glow);
		}
	}
	return VERY_LONG_TICK;
}

void MapSpatialComponent::SetMapPosition( Chit* chit, int x, int y )
{
	GLASSERT( chit);
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	GLASSERT(msc);
	int size = msc->Size();

	GLASSERT( size <= MAX_BUILDING_SIZE );
	GLASSERT( size <= MAX_BUILDING_SIZE );
	
	float px = (float)x + float(size)*0.5f;
	float py = (float)y + float(size)*0.5f;

	chit->SetPosition( px, 0, py );
}


void MapSpatialComponent::SetBlocks( bool value ) 
{
	blocks = value;	// UpdateBlock() makes callback occur - set mode first!
	SyncWithSpatial();
}


void MapSpatialComponent::SetBuilding( int size, bool p, int circuit )
{
	GLASSERT(!parentChit);	// not sure this works after add.

	this->size = size;
	this->hasPorch = p ? 1 : 0;
	this->hasCircuit = circuit;

	SyncWithSpatial();	// probably does nothing. this really shouldn't be called after add.
}


/*static*/ void MapSpatialComponent::UpdateGridLayer(WorldMap* worldMap, LumosChitBag* chitBag, CircuitSim* ciruitSim, const Rectangle2I& rect)
{
	bool circuitChange = false;
	for (Rectangle2IIterator it(rect); !it.Done(); it.Next()) {
		int porchType = 0;
		Chit* porchChit = chitBag->QueryPorch(it.Pos());
		if (porchChit) {
			MapSpatialComponent* msc = GET_SUB_COMPONENT(porchChit, SpatialComponent, MapSpatialComponent);
			GLASSERT(msc);
			porchType = msc->PorchType();
		}
		worldMap->SetPorch(it.Pos().x, it.Pos().y, porchType);

		int circuit = 0;
		float yRotation = 0;

		Chit* chit = chitBag->QueryBuilding(IString(), it.Pos(), 0);
		if (chit) {
			MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
			if (msc) {
				circuit = msc->CircuitType();
				yRotation = YRotation(chit->Rotation());
			}
		}

		const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos());
		if (circuit) {
			int r = LRint(yRotation / 90.0f);
			if (wg.CircuitRot() != r) {
				worldMap->SetCircuitRotation(it.Pos().x, it.Pos().y, r );
				circuitChange = true;
			}
		}
		if (circuit != wg.Circuit()) {
			circuitChange = true;
			worldMap->SetCircuit(it.Pos().x, it.Pos().y, circuit);
		}
	}
	if (ciruitSim && circuitChange) {
		ciruitSim->EtchLines(ToSector(rect.min));
	}
}


void MapSpatialComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
	SyncWithSpatial();
}


void MapSpatialComponent::OnRemove()
{
	Context()->chitBag->RemoveFromBuildingHash(this, bounds.min.x, bounds.min.y);
	
	WorldMap* worldMap = Context()->worldMap;
	LumosChitBag* chitBag = Context()->chitBag;
	CircuitSim* circuitSim = Context()->circuitSim;

	super::OnRemove();

	// Since we are removed from the HashTable, this
	// won't be found by UpdateGridLayer()
	Rectangle2I b = bounds;
	b.Outset(1);
	worldMap->UpdateBlock(bounds);
	UpdateGridLayer(worldMap, chitBag, circuitSim, b);
}


void MapSpatialComponent::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, "MapSpatialComponent");
	XARC_SER(xs, size);
	XARC_SER(xs, blocks);
	XARC_SER(xs, hasPorch);
	XARC_SER(xs, hasCircuit);
	XARC_SER(xs, glow);
	XARC_SER(xs, glowTarget);
	this->EndSerialize(xs);
}


Rectangle2I MapSpatialComponent::PorchPos() const
{
	Rectangle2I v;
	v.Set( 0, 0, 0, 0 );
	if ( !hasPorch ) return v;

	float yRotation = YRotation(parentChit->Rotation());
	v = CalcPorchPos(bounds.min, bounds.Width(), yRotation);
	return v;
}


Rectangle2I MapSpatialComponent::CalcPorchPos(const Vector2I& pos, int size, float rotation)
{
	Rectangle2I b;
	b.min = b.max = pos;
	b.max.x += (size - 1);
	b.max.y += (size - 1);

	int r = LRintf(rotation / 90.0f);
	Rectangle2I v = b;

	switch (r) {
		case 0:		v.min.y = v.max.y = b.max.y + 1;	break;
		case 1:		v.min.x = v.max.x = b.max.x + 1;	break;
		case 2:		v.min.y = v.max.y = b.min.y - 1;	break;
		case 3:		v.min.x = v.max.x = b.min.x - 1;	break;
		default:	GLASSERT(0);	break;
	}

	return v;
}
