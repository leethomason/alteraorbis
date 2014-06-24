#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"

#include "../engine/engine.h"
#include "../engine/model.h"

using namespace grinliz;

static const float ELECTRON_SPEED = DEFAULT_MOVE_SPEED * 1.0f;

static const Vector2F DIR_F4[4] = {
	{ 1, 0 },
	{ 0, 1 },
	{ -1, 0 },
	{ 0, -1 },
};

static const Vector2I DIR_I4[4] = {
	{ 1, 0 },
	{ 0, 1 },
	{ -1, 0 },
	{ 0, -1 },
};


CircuitSim::CircuitSim(WorldMap* p_worldMap, Engine* p_engine)
{
	worldMap = p_worldMap;
	engine = p_engine;
	GLASSERT(worldMap);
	GLASSERT(engine);
}


CircuitSim::~CircuitSim()
{
	for (int i = 0; i < electrons.Size(); ++i) {
		engine->FreeModel(electrons[i].model);
	}
	electrons.Clear();
}


void CircuitSim::Activate(const grinliz::Vector2I& pos)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos)];

	if (wg.Circuit() == WorldGrid::CIRCUIT_SWITCH) {
		CreateSpark(pos, wg.CircuitRot());
	}
}


void CircuitSim::CreateSpark(const grinliz::Vector2I& pos, int rot4)
{
	Model* model = engine->AllocModel("spark");
	Vector2F start = ToWorld2F(pos) + 0.1f * DIR_F4[rot4];
	model->SetPos(ToWorld3F(pos));

	Electron e = { 0, rot4, 0.01f, pos, model };
	electrons.Push(e);
}


void CircuitSim::DoTick(U32 delta)
{
	for (int i = 0; i < electrons.Size(); ++i) {
		Electron* pe = &electrons[i];

		float travel = Travel(ELECTRON_SPEED, delta);
		bool electronDone = false;

		while (travel > 0) {
			if (pe->t < 0) {
				float d = fabs(pe->t);
				// Center is next!
				if (travel < d) {
					// but can't reach center:
					pe->t += travel;
					travel = 0;
				}
				else {
					// Hit center!
					travel -= fabs(pe->t);
					pe->t = 0;
					electronDone = SparkArrives(pe->pos);
					if (electronDone) travel = 0;
				}
			}
			else {
				// Next edge!
				float d = fabs(0.5f - pe->t);
				if (travel >= d) {
					pe->pos += DIR_I4[pe->dir];
					pe->t = -0.5f;
					travel -= d;

					if (!worldMap->Bounds().Contains(pe->pos)) {
						travel = 0;
						electronDone = true;
					}
					else {
						const WorldGrid& wg = worldMap->GetWorldGrid(pe->pos);
						if (wg.IsGrid() || wg.IsPort()) {
							travel = 0;
							electronDone = true;
							EmitSparkExplosion(ToWorld2F(pe->pos) - 0.5f * DIR_F4[pe->dir]);
						}
					}
				}
				else {
					pe->t += travel;
					travel = 0;
				}
			}
		}
		if (electronDone) {
			engine->FreeModel(pe->model);
			electrons.SwapRemove(i); --i;
		}
		else {
			Vector2F p2 = ToWorld2F(pe->pos) + pe->t * DIR_F4[pe->dir];
			Vector3F p3 = { p2.x, 0, p2.y };
			pe->model->SetPos(p3);
		}
	}
}


bool CircuitSim::SparkArrives(const grinliz::Vector2I& pos)
{
	return false;
}


void CircuitSim::EmitSparkExplosion(const grinliz::Vector2F& pos)
{

}
