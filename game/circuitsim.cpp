#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/particle.h"

#include "../script/battlemechanics.h"
#include "../script/batterycomponent.h"

#include "../xegame/spatialcomponent.h"

using namespace grinliz;

static const float ELECTRON_SPEED = DEFAULT_MOVE_SPEED * 6.0f;
static const float DAMAGE_PER_CHARGE = 15.0f;

static const Vector2I DIR_I4[4] = {
	{ 0, -1 },
	{ -1, 0 },
	{ 0, 1 },
	{ 1, 0 },
};


static const Vector2F DIR_F4[4] = {
	{ (float)DIR_I4[0].x, (float)DIR_I4[0].y },
	{ (float)DIR_I4[1].x, (float)DIR_I4[1].y },
	{ (float)DIR_I4[2].x, (float)DIR_I4[2].y },
	{ (float)DIR_I4[3].x, (float)DIR_I4[3].y },
};


CircuitSim::CircuitSim(WorldMap* p_worldMap, Engine* p_engine, LumosChitBag* p_chitBag)
{
	worldMap = p_worldMap;
	engine = p_engine;
	chitBag = p_chitBag;
	GLASSERT(worldMap);
	GLASSERT(engine);
	// the chitBag can be null.
	if (chitBag) {
		chitBag->AddListener(this);
	}
}


CircuitSim::~CircuitSim()
{
	for (int i = 0; i < electrons.Size(); ++i) {
		engine->FreeModel(electrons[i].model);
	}
	electrons.Clear();
	if (chitBag) {
		chitBag->RemoveListener(this);
	}
}


void CircuitSim::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::TRIGGER_DETECTOR_CIRCUIT) {
		if (chit->GetSpatialComponent()) {
			TriggerSwitch(chit->GetSpatialComponent()->GetPosition2DI());
		}
	}
}


void CircuitSim::TriggerSwitch(const grinliz::Vector2I& pos)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos)];

	if (wg.Circuit() == CIRCUIT_SWITCH) {
		CreateElectron(pos, wg.CircuitRot(), 0);
	}
}


void CircuitSim::TriggerDetector(const grinliz::Vector2I& pos)
{
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos)];

	if (wg.Circuit() == CIRCUIT_DETECT_SMALL_ENEMY || wg.Circuit() == CIRCUIT_DETECT_LARGE_ENEMY) {
		CreateElectron(pos, wg.CircuitRot(), 0);
	}
}


void CircuitSim::CreateElectron(const grinliz::Vector2I& pos, int rot4, int charge)
{
	Model* model = engine->AllocModel( charge ? "charge" : "spark");
	Vector2F start = ToWorld2F(pos);
	model->SetPos(ToWorld3F(pos));

	Electron e = { charge, rot4, 0.01f, pos, model };
	electrons.Push(e);
}


int CircuitSim::NameToID(grinliz::IString name)
{
	if (name == "battery") return CIRCUIT_BATTERY;
	else if (name == "powerUp") return CIRCUIT_POWER_UP;
	return CIRCUIT_NONE;
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
	int dir = (pe->dir - wg.CircuitRot() + 4) & 3;

	switch (wg.Circuit()) {
		case CIRCUIT_SWITCH: {
			// Activates the switch., consumes the charge.
			TriggerSwitch(pe->pos);
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_BATTERY: {
			if (dir == 0 || pe->charge == 0) {
				// The working direction, or a spark from anywhere.
				if (chitBag) {
					Chit* power = chitBag->QueryBuilding(pe->pos);
					if (power) {
						BatteryComponent* battery = (BatteryComponent*)power->GetComponent("BatteryComponent");
						if (battery) {
							pe->charge += battery->UseCharge();
						}
					}
				}
				else {
					pe->charge += 4;
				}
				pe->dir = wg.CircuitRot();
				dir = 0;	// so we don't go into the "explode" case.
			}
			if (pe->charge > 8 || (dir && pe->charge)) {
				// too much power, or power from the side.
				Explosion(pe->pos, pe->charge, false);	// if destroyed, removed by building
				sparkConsumed = true;
			}
		}
		break;

		case CIRCUIT_POWER_UP: {
			ApplyPowerUp(pe->pos, pe->charge);
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_BEND:
		if (dir == 0) pe->dir = ((pe->dir + 1) & 3);
		else if (dir == 3) pe->dir = ((pe->dir + 3) & 3);
		break;

		case CIRCUIT_FORK_2:
		{
			CreateElectron(pe->pos, ((pe->dir + 1) & 3), pe->charge/2);
			CreateElectron(pe->pos, ((pe->dir + 3) & 3), pe->charge/2);
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_ICE:
		{
			sparkConsumed = true;
			if (pe->charge) {
				const WorldGrid& wg = worldMap->GetWorldGrid(pe->pos);
				worldMap->SetRock(pe->pos.x, pe->pos.y, wg.RockHeight() ? 0 : 1, false, WorldGrid::ICE);
			}
		}
		break;

		case CIRCUIT_STOP:
		{
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_TRANSISTOR_A:
		case CIRCUIT_TRANSISTOR_B:
		{
			if (dir == 0) {
				worldMap->SetCircuit(pe->pos.x, pe->pos.y, 
									(wg.Circuit() == CIRCUIT_TRANSISTOR_A) ? CIRCUIT_TRANSISTOR_B : CIRCUIT_TRANSISTOR_A);
				sparkConsumed = true;
			}
			else if (pe->charge && dir == 2) {
				pe->dir = pe->dir + ((wg.Circuit() == CIRCUIT_TRANSISTOR_A) ? 3 : 1);
				pe->dir = pe->dir & 3;
			}
			else {
				sparkConsumed = true;
			}
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
	if (chitBag) {
		BattleMechanics::GenerateExplosion(dd, pos3, 0, engine, chitBag, worldMap);
	}
	if (circuitDestroyed) {
		worldMap->SetCircuit(pos.x, pos.y, 0);
	}
}

