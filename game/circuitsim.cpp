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
#include "../xegame/rendercomponent.h"
#include "../xegame/game.h"
#include "../audio/xenoaudio.h"

using namespace grinliz;

static const float ELECTRON_SPEED = DEFAULT_MOVE_SPEED * 6.0f;
static const float DAMAGE_PER_CHARGE = 15.0f;

static const Vector2I DIR_I4[4] = {
	{ 0, 1 },
	{ 1, 0 },
	{ 0, -1 },
	{ -1, 0 },
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
}


CircuitSim::~CircuitSim()
{
	for (int i = 0; i < electrons.Size(); ++i) {
		engine->FreeModel(electrons[i].model);
	}
	electrons.Clear();
}


void CircuitSim::Electron::Serialize(XStream* xs)
{
	XarcOpen(xs, "Electron");
	XARC_SER(xs, charge);
	XARC_SER(xs, dir);
	XARC_SER(xs, t);
	XARC_SER(xs, pos);
	XarcClose(xs);
}


void CircuitSim::Serialize(XStream* xs)
{
	XarcOpen(xs, "CircuitSim");
	XARC_SER_CARRAY(xs, electrons);
	XarcClose(xs);
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

	if (wg.Circuit() == CIRCUIT_DETECT_ENEMY ) {
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
	int ePerSector[NUM_SECTORS*NUM_SECTORS] = { 0 };

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

		Vector2I sector = ToSector(pe->pos);
		int sectorIndex = sector.y*NUM_SECTORS + sector.x;
		ePerSector[sectorIndex] += 1;
		if (ePerSector[sectorIndex] > 10) {
			// Very easy to create infinite loops, infinite
			// generators, etc. Even good reason to do so.
			electronDone = true;
			travel = 0;
		}

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
			if (!pe->model || !StrEqual(pe->model->GetResource()->Name(), resName)) {
				engine->FreeModel(pe->model);
				pe->model = engine->AllocModel(resName);
			}

			ParticleDef pd = engine->particleSystem->GetPD("electron");
			if (pe->charge)
				pd.color = Game::GetMainPalette()->Get4F(5, 3);
			else
				pd.color = Game::GetMainPalette()->Get4F(7, 4);

			engine->particleSystem->EmitPD(pd, pe->model->Pos(), V3F_UP, 0);
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
			if (pe->charge) {
				ApplyPowerUp(pe->pos, pe->charge);
			}
			sparkConsumed = true;
		}
		break;

		case CIRCUIT_BEND:
		if (dir == 2) pe->dir = ((pe->dir + 3) & 3);
		else if (dir == 3) pe->dir = ((pe->dir + 1) & 3);
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
			if (dir == 2) {
				worldMap->SetCircuit(pe->pos.x, pe->pos.y, 
									(wg.Circuit() == CIRCUIT_TRANSISTOR_A) ? CIRCUIT_TRANSISTOR_B : CIRCUIT_TRANSISTOR_A);
				sparkConsumed = true;
			}
			else if (dir == 0) {
				pe->dir = pe->dir + ((wg.Circuit() == CIRCUIT_TRANSISTOR_A) ? 1 : 3);
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
		DamageDesc dd(float(5 * charge), GameItem::EFFECT_SHOCK);

		Chit* building = chitBag->QueryBuilding(pos);
		if (building && building->GetItem() && building->GetItem()->IName() == ISC::turret && building->GetRenderComponent()) {
			Quaternion q = building->GetRenderComponent()->MainModel()->GetRotation();
			Matrix4 rot;
			q.ToMatrix(&rot);
			Vector3F straight = rot * V3F_OUT;
			RenderComponent* rc = building->GetRenderComponent();
			Vector3F trigger = { 0, 0, 0 };
			rc->CalcTrigger(&trigger, 0);

			XenoAudio::Instance()->PlayVariation(ISC::blaster, building->ID(), &trigger);

			chitBag->NewBolt(trigger, straight, dd.effects, building->ID(),
							 dd.damage,
							 8.0f,
							 false);
		}
		else {
			CChitArray arr;
			MOBIshFilter filter;
			chitBag->QuerySpatialHash(&arr, ToWorld2F(pos), 0.5f, 0, &filter);

			ChitDamageInfo info( dd );
			info.originID = building->ID();
			info.awardXP  = false;
			info.isMelee  = false;
			info.isExplosion = false;
			info.originOfImpact = V3F_ZERO;

			ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &info );

			for (int i = 0; i < arr.Size(); ++i) {
				arr[i]->SendMessage(msg, 0);
			}
		}
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

