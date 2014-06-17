#include "gamemovecomponent.h"
#include "worldmap.h"
#include "../Shiny/include/Shiny.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

static const float FLUID_SPEED = DEFAULT_MOVE_SPEED * 0.5f;

void GameMoveComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


void GameMoveComponent::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());
	this->EndSerialize(xs);
}


int GameMoveComponent::DoTick(U32 delta)
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if (!sc) return VERY_LONG_TICK;

	Vector3F pos = sc->GetPosition();
	Vector3F start = pos;

	// Basic physics and block avoidance.
	bool floating = false;
	bool appliedFluid = ApplyFluid(delta, &pos, &floating);

	if (!floating) {
		Vector2F pos2 = ToWorld2F(pos);
		bool force = false;
		ApplyBlocks(&pos2, &force);
		pos.x = pos2.x;
		pos.z = pos2.y;
	}

	if (start != pos) {
		sc->SetPosition(pos);
		return 0;
	}
	return VERY_LONG_TICK;
}


void GameMoveComponent::ApplyBlocks( Vector2F* pos, bool* forceApplied )
{
	//GRINLIZ_PERFTRACK;
	PROFILE_FUNC();

	RenderComponent* render = parentChit->GetRenderComponent();
	GLASSERT( render );
	if ( !render ) return;

	Vector2F newPos = *pos;
	float rotation = 0;
	float radius = render->RadiusOfBase();

	const ChitContext* context = Context();

	WorldMap::BlockResult result = context->worldMap->ApplyBlockEffect( *pos, radius, WorldMap::BT_PASSABLE, &newPos );
	if ( forceApplied ) *forceApplied = ( result == WorldMap::FORCE_APPLIED );
	bool isStuck = ( result == WorldMap::STUCK );

	if ( forceApplied || isStuck ) {
		int x = (int)newPos.x;
		int y = (int)newPos.y;

		if ( !context->worldMap->IsPassable( x, y )) {
  			Vector2I out = context->worldMap->FindPassable( x, y );
			newPos.x = (float)out.x + 0.5f;
			newPos.y = (float)out.y + 0.5f;
		}
	}
	
	*pos = newPos;
}


bool GameMoveComponent::ApplyFluid(U32 delta, grinliz::Vector3F* pos, bool* floating)
{
	WorldGrid wg[5];
	Vector2I pos2i = ToWorld2I(*pos);
	Vector2I dir[5];

	Context()->worldMap->GetWorldGrid(pos2i, wg, 5, dir);
	if (!wg[0].IsFluid()) {
		return false;
	}

	// Compute gradient.
	Vector2F grad = { 0, 0 };

	float v0 = wg[0].FluidHeight();
	float vPos1 = wg[1].IsFluid() ? wg[1].FluidHeight() : v0;
	float vNeg1 = wg[3].IsFluid() ? wg[3].FluidHeight() : v0;
	grad.x = ((vPos1 - v0) + (v0 - vNeg1)) * 0.5f;

	vPos1 = wg[2].IsFluid() ? wg[2].FluidHeight() : v0;
	vNeg1 = wg[4].IsFluid() ? wg[4].FluidHeight() : v0;
	grad.y = ((vPos1 - v0) + (v0 - vNeg1)) * 0.5f;

	/*
	float dx = pos->x - (float(pos2i.x) + 0.5f);
	float dy = pos->z - (float(pos2i.y) + 0.5f);

	float a, b, c;
	float v0 = wg[0].FluidHeight();
	float vPos1 = wg[1].IsFluid() ? wg[1].FluidHeight() : v0;
	float vNeg1 = wg[3].IsFluid() ? wg[3].FluidHeight() : v0;
	
	// a2v + bv + c
	QuadCurveFit11(vNeg1, wg[0].FluidHeight(), vPos1, &a, &b, &c);
	// 1st deriv: 2av + b
	grad.x = 2.0f*a*dx + b;

	vPos1 = wg[2].IsFluid() ? wg[2].FluidHeight() : v0;
	vNeg1 = wg[4].IsFluid() ? wg[4].FluidHeight() : v0;
	QuadCurveFit11(vNeg1, v0, vPos1, &a, &b, &c);
	grad.y = 2.0f*a*dy + b;
	*/
	// Floating??
	*floating = false;
	float h = 0;
	RenderComponent* rc = parentChit->GetRenderComponent();
	ItemComponent* ic = parentChit->GetItemComponent();
	if (rc && ic && ic->GetItem()->flags & GameItem::FLOAT) {
		h = rc->MainResource()->AABB().SizeY();
		if (wg[0].FluidHeight() > h*0.5f) {
			*floating = true;
		}
	}

	float SPEED = *floating ? FLUID_SPEED : FLUID_SPEED * 0.5f;
	float speed = Travel(SPEED, delta);

	// Move in 2 steps to stay in fluid:
	pos->x += -speed * grad.x;
	pos->y = 0;
	pos->z += -speed * grad.y;

	Vector2F pos2 = { pos->x, pos->z };
	Context()->worldMap->ApplyBlockEffect(pos2, MAX_BASE_RADIUS, *floating ? WorldMap::BT_FLUID : WorldMap::BT_PASSABLE, &pos2);
	pos->x = pos2.x;
	pos->z = pos2.y;

	// If we are floating, set the floating height.
	if (*floating) {
		const WorldGrid& newWG = Context()->worldMap->GetWorldGrid(ToWorld2I(pos2));
		if (newWG.IsFluid()) {
			pos->y = newWG.FluidHeight() - h * 0.5f;
		}
	}
	return true;
}
