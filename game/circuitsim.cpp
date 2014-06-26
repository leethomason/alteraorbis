#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/particle.h"

#include "../script/battlemechanics.h"
#include "../script/batterycomponent.h"

using namespace grinliz;

static const float ELECTRON_SPEED = DEFAULT_MOVE_SPEED * 4.0f;
static const float DAMAGE_PER_CHARGE = 15.0f;

static const Vector2F DIR_F4[4] = {
	{ 1, 0 },
	{ 0, -1 },
	{ -1, 0 },
	{ 0, 1 },
};

static const Vector2I DIR_I4[4] = {
	{ 1, 0 },
	{ 0, -1 },
	{ -1, 0 },
	{ 0, 1 },
};


CircuitSim::CircuitSim(WorldMap* p_worldMap, Engine* p_engine, LumosChitBag* p_chitBag)
{
	worldMap = p_worldMap;
	engine = p_engine;
	chitBag = p_chitBag;
	GLASSERT(worldMap);
	GLASSERT(engine);
	// the chitBag can be null.
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

	if (wg.Circuit() == CIRCUIT_SWITCH) {
		CreateElectron(pos, wg.CircuitRot(), 0);
	}
}


void CircuitSim::CreateElectron(const grinliz::Vector2I& pos, int rot4, int charge)
{
	Model* model = engine->AllocModel( charge ? "charge" : "spark");
	Vector2F start = ToWorld2F(pos) + 0.1f * DIR_F4[rot4];
	model->SetPos(ToWorld3F(pos));

	Electron e = { 0, rot4, 0.01f, pos, model };
	electrons.Push(e);
}


void CircuitSim::DoTick(U32 delta)
{
	// This is a giant "mutate what we are iterating" problem.
	// Copy for now; worry about perf later.
	electronsCopy.Clear();
	while (!electrons.Empty()) {
		electronsCopy.Push(electrons.Pop());
	}

	for (int i = 0; i < electronsCopy.Size(); ++i) {
		Electron* pe = &electronsCopy[i];

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
					electronDone = ElectronArrives(pe);
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
			// just don't push back to the 'electrons' array to delete it.
		}
		else {
			Vector2F p2 = ToWorld2F(pe->pos) + pe->t * DIR_F4[pe->dir];
			Vector3F p3 = { p2.x, 0, p2.y };

			const char* resName = pe->charge ? "charge" : "spark";
			if (!StrEqual(pe->model->GetResource()->Name(), resName)) {
				engine->FreeModel(pe->model);
				pe->model = engine->AllocModel(resName);
			}

			pe->model->SetPos(p3);
			electrons.Push(*pe);
		}
	}
}


bool CircuitSim::ElectronArrives(Electron* pe)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pe->pos)];
	bool sparkConsumed = false;
	int dir = (pe->dir - wg.CircuitRot() + 4) & 3; // Test: correct?

	switch (wg.Circuit()) {
		case CIRCUIT_SWITCH: {
			// Activates the switch., consumes the charge.
			Activate(pe->pos);
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_BATTERY: {
			if (dir == 0) {
				// The working direction.
				if (chitBag) {
					Chit* power = chitBag->QueryBuilding(pe->pos);
					if (power) {
						BatteryComponent* battery = (BatteryComponent*)power->GetComponent("BatteryComponent");
						if (battery) {
							pe->charge = battery->UseCharge();
						}
					}
				}
				else {
					pe->charge += 4;
				}
			}
			if (pe->charge > 8 || (dir && pe->charge)) {
				// too much power, or power from the side.
				Explosion(pe->pos, pe->charge, false);	// if destroyed, removed by building
				sparkConsumed = true;
			}
			else if (dir && pe->charge == 0) {
				// sideways spark.
				sparkConsumed = true;
			}
		}
		break;

		case CIRCUIT_POWER_UP: {
			ApplyPowerUp(pe->pos, pe->charge);
			sparkConsumed = true;
		}
		break;

		default:
		break;
	}

	return sparkConsumed;
}


void CircuitSim::EmitSparkExplosion(const grinliz::Vector2F& pos)
{
	Vector3F p3 = { pos.x, 0, pos.y };
	engine->particleSystem->EmitPD("sparkExplosion", p3, V3F_UP, 0);
}


void CircuitSim::ApplyPowerUp(const grinliz::Vector2I& pos, int charge)
{
	if (chitBag) {
		GLASSERT(0);	// code not written: look for turret or gate, etc. or find mobs to zap
	}
	else {
		engine->particleSystem->EmitPD("sparkPowerUp", ToWorld3F(pos), V3F_UP, 0);
	}
}


void CircuitSim::Explosion(const grinliz::Vector2I& pos, int charge, bool circuitDestroyed)
{
	Vector3F pos3 = ToWorld3F(pos);
	DamageDesc dd = { float(charge) * DAMAGE_PER_CHARGE, GameItem::EFFECT_SHOCK };
	BattleMechanics::GenerateExplosion(dd, pos3, 0, engine, chitBag, worldMap);
	if (circuitDestroyed) {
		worldMap->SetCircuit(pos.x, pos.y, 0);
	}
}

