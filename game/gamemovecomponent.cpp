#include "gamemovecomponent.h"
#include "worldmap.h"
#include "../Shiny/include/Shiny.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

// Multiplied by a gradient: not actually this fast.
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

	velocity = (pos - start) * (1000.0f / float(delta));
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

	WorldMap::BlockResult br = context->worldMap->ApplyBlockEffect( *pos, radius, WorldMap::BT_PASSABLE, &newPos );
	if (forceApplied) {
		*forceApplied = (br == WorldMap::FORCE_APPLIED) ? true : false;
	}

	*pos = newPos;
}


bool GameMoveComponent::ApplyFluid(U32 delta, grinliz::Vector3F* pos, bool* floating)
{
	static const int N = 9;
	if (delta > MAX_FRAME_TIME) delta = MAX_FRAME_TIME;

	WorldGrid wg[N];
	Vector2I pos2i = ToWorld2I(*pos);
	Vector2I dir[N];

	Context()->worldMap->GetWorldGrid(pos2i, wg, N, dir);
	if (!wg[0].IsFluid()) {
		return false;
	}

	// Compute gradient.
	Vector2F grad = { 0, 0 };

	float v0 = wg[0].FluidHeight();
	
	for (int i = 1; i < N; ++i) {
		Vector2F delta = { (float)dir[i].x, (float)dir[i].y };
		if (dir[i].x && dir[i].y) delta = delta * 0.7f;

		float v = (wg[i].IsFluid() || wg[i].Height() == 0) ? wg[i].FluidHeight() : v0;

		grad = grad + delta * v;
	}

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

	bool submarine = false;
	if (parentChit->GetItem()) {
		submarine = (parentChit->GetItem()->flags & GameItem::SUBMARINE) != 0;
	}

	float SPEED = *floating ? FLUID_SPEED : FLUID_SPEED * 0.5f;
	float speed = Travel(SPEED, delta);

	if (!submarine) {
		// Move in 2 steps to stay in fluid:
		pos->x += -speed * grad.x;
		pos->y = 0;
		pos->z += -speed * grad.y;
	}

	Vector2F pos2 = { pos->x, pos->z };
	GLASSERT(Context()->worldMap->BoundsF().Contains(pos2));
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
