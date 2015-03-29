#include "circuitsim.h"
#include "worldmap.h"
#include "lumosmath.h"
#include "lumoschitbag.h"
#include "lumosgame.h"
#include "mapspatialcomponent.h"
#include "gameitem.h"

#include "../engine/particle.h"
#include "../engine/engine.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"

#include "../Shiny/include/Shiny.h"

#include "../script/procedural.h"
#include "../gamui/gamui.h"
#include "../audio/xenoaudio.h"

using namespace grinliz;
using namespace gamui;

CircuitSim::CircuitSim(const ChitContext* _context) : context(_context)
{
	GLASSERT(context);

	RenderAtom groupColor[NUM_GROUPS] = {
		LumosGame::CalcPaletteAtom(PAL_TANGERINE * 2, PAL_TANGERINE), // power
		LumosGame::CalcPaletteAtom(PAL_BLUE * 2, PAL_BLUE), // sensor
		LumosGame::CalcPaletteAtom(PAL_GREEN * 2, PAL_GREEN), // device
	};
	for (int i = 0; i < NUM_GROUPS; ++i) {
		canvas[i].Init(&context->worldMap->overlay1, groupColor[i]);
		canvas[i].SetLevel(-10+i);
	}
	visible = true;
	roundRobbin = 0;
}


CircuitSim::~CircuitSim()
{
}

void CircuitSim::Serialize(XStream* xs)
{
	XarcOpen(xs, "CircuitSim");
	XarcClose(xs);
}


void CircuitSim::TriggerSwitch(const grinliz::Vector2I& pos)
{
	WorldMap* worldMap = context->worldMap;
	const WorldGrid& wg = worldMap->grid[worldMap->INDEX(pos)];

//	if (wg.Circuit() == CIRCUIT_SWITCH) {
//	}
}


void CircuitSim::NewParticle(EParticleType type, int powerRequest, const grinliz::Vector2F& origin, const grinliz::Vector2F& dest, int delay)
{
	Particle p = { type, powerRequest, origin, origin, dest, delay };
	newQueue.Push(p);
}


Vector2F CircuitSim::FindPower(const Group& group)
{
	Vector2F pos = { 0, 0 };
	for (const Connection* c = connections.begin(); c < connections.end(); ++c) {
		if (c->type == POWER_GROUP && group.bounds.Contains(c->a))  {
			pos = ToWorld2F(c->b);
			break;
		}
		else if (c->type == POWER_GROUP && group.bounds.Contains(c->b)) {
			pos = ToWorld2F(c->a);
			break;
		}
	}
	return pos;
}


void CircuitSim::FindConnections(const Group& group, grinliz::CDynArray<const Connection*> *out)
{
	for (const Connection& c : connections) {
		if (group.bounds.Contains(c.a) || group.bounds.Contains(c.b)) {
			out->Push(&c);
		}
	}
}


void CircuitSim::FireTurret(int id)
{
	Chit* building = context->chitBag->GetChit(id);
	if (!building) return;
	if (!building->GetRenderComponent()) return;
	if (!building->GetItem()) return;
	if (building->GetItem()->IName() != ISC::turret) return;

	Quaternion q = building->GetRenderComponent()->MainModel()->GetRotation();
	Matrix4 rot;
	q.ToMatrix(&rot);
	Vector3F straight = rot * V3F_OUT;
	RenderComponent* rc = building->GetRenderComponent();
	Vector3F trigger = { 0, 0, 0 };
	rc->CalcTrigger(&trigger, 0);

	XenoAudio::Instance()->PlayVariation(ISC::blasterWAV, building->ID(), &trigger);

	DamageDesc dd(15, GameItem::EFFECT_SHOCK);
	context->chitBag->NewBolt(trigger, straight, dd.effects, building->ID(),
							  dd.damage, 8.0f, false);
}

/*
	All connections are between a Device and something; so
	the type of connection is the not-device one. (Power or Sensor.)
	
	1. A sensor sends control particles to ALL attatched devices.
	2. When a device receives a control particle, sends a control particle with N requests to ONE attatched power.
	3. When a device receives a power particle, it activates. (round robin in group)
	4. When a power receives a control particle, it queues N power particles.
*/

void CircuitSim::ParticleArrived(const Particle& p)
{
	static const int POWER_DELAY = 250;

	int type = 0, index = 0;
	if (FindGroup(ToWorld2I(p.dest), &type, &index)) {
		const Group& group = groups[type][index];
		if (type == DEVICE_GROUP && p.type == EParticleType::control) {
			Vector2F power = FindPower(group);
			if (!power.IsZero()) {
				// request power
				NewParticle(EParticleType::control, group.idArr.Size(), p.pos, power);
			}
		}
		else if (type == DEVICE_GROUP && p.type == EParticleType::power) {
			// activate device
			int id = group.idArr[(roundRobbin++) % group.idArr.Size()];
			FireTurret(id);
		}
		else if (type == POWER_GROUP && p.type == EParticleType::control) {
			for (int i = 0; i < p.powerRequest; ++i) {
				NewParticle(EParticleType::power, 0, p.dest, p.origin, POWER_DELAY*i);
			}
		}
	}
}


void CircuitSim::TriggerDetector(const grinliz::Vector2I& pos)
{
	Chit* building = context->chitBag->QueryBuilding(IString(), pos, 0);
	if (building) {
		const GameItem* item = building->GetItem();
		if (item) {
			if (item->IName() == ISC::detector) {
				int type = 0, index = 0;
				if (FindGroup(pos, &type, &index) && type == SENSOR_GROUP) {
					const Group& group = groups[type][index];
					FindConnections(group, &queryConn);
					for (const Connection* c : queryConn) {
						Vector2I a = c->a;
						Vector2I b = c->b;
						if (!group.bounds.Contains(a)) {
							Swap(&a, &b);
						}

						Vector2F start = ToWorld2F(a);
						Vector2F end = ToWorld2F(b);
						NewParticle(EParticleType::control, 0, start, end, 0);
					}
				}
			}
		}
	}
}


void CircuitSim::DoTick(U32 delta)
{
	static const float SPEED = 2.0f;
	float travel = Travel(SPEED, delta);

	// Once again...don't mutate the array we are iterating on. *sigh*
	while (!newQueue.Empty()) {
		particles.Push(newQueue.Pop());
	}

	for (int i = 0; i < particles.Size(); ++i) {
		Particle& p = particles[i];
		if (p.delay > 0) {
			p.delay -= int(delta);
			if (p.delay < 0) p.delay = 0;
			continue;
		}

		float len = (p.pos - p.dest).Length();
		if (len < travel) {
			ParticleArrived(p);
			particles.SwapRemove(i);
			--i;
		}
		else {
			static const int NPART = 3;
			static const float fraction = 1.0f / float(NPART);
			Vector2F normal = p.dest - p.pos;
			normal.Normalize();

			for (int j = 0; j < NPART; ++j) {
				Vector2F pos = p.pos + normal * (travel * float(j + 1)*fraction);
				IString particle = (p.type == EParticleType::control) ? ISC::control : ISC::power;
				context->engine->particleSystem->EmitPD(particle, ToWorld3F(pos), V3F_UP, delta);
			}
			p.pos += normal * travel;
		}
	}
}


void CircuitSim::FillGroup(Group* g, const Vector2I& pos, Chit* chit)
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	GLASSERT(msc);

	g->bounds.DoUnion(msc->Bounds());
	g->idArr.PushIfCap(chit->ID());
	hashTable.Remove(pos);
	for (int i = 0; i < 4; ++i) {
		Vector2I nextPos = pos.Adjacent(i);
		Chit* nextChit = 0;
		if (hashTable.Query(nextPos, &nextChit)) {
			FillGroup(g, nextPos, nextChit);
		}
	}
}


void CircuitSim::CalcGroups(const grinliz::Vector2I& sector)
{
	IString powerNames[]  = { ISC::temple, IString() };
	bool powerGroups[] = { false, false };
	IString sensorNames[] = { ISC::detector, ISC::switchOn, ISC::switchOff, IString() };
	bool sensorGroups[] = { true, false, false, false };
	IString deviceNames[] = { ISC::turret, ISC::gate, IString() };
	bool deviceGroups[] = { true, true };

	IString* names[NUM_GROUPS] = { powerNames, sensorNames, deviceNames };
	const bool* doesGroup[NUM_GROUPS] = { powerGroups, sensorGroups, deviceGroups };

	hashTable.Clear();
	for (int i = 0; i < NUM_GROUPS; ++i) {
		groups[i].Clear();
		combinedArr.Clear();

		for (int j = 0; !names[i][j].empty(); ++j) {
			context->chitBag->FindBuilding(names[i][j], sector, 0, LumosChitBag::EFindMode::NEAREST, &queryArr, 0);
			if (doesGroup[i][j]) {
				for (Chit* chit : queryArr) {
					Vector2I pos = ToWorld2I(chit->Position());
					hashTable.Add(pos, chit);
					combinedArr.Push(chit);
				}
			}
			else {
				for (Chit* chit : queryArr) {
					MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
					GLASSERT(msc);
					Group* g = groups[i].PushArr(1);
					g->bounds = msc->Bounds();
					g->idArr.Push(chit->ID());
				}
			}
		}

		for (Chit* chit : combinedArr) {
			// If it is still in the hash table, then
			// it is a group.
			Vector2I pos = ToWorld2I(chit->Position());
			if (hashTable.Query(pos, 0)) {
				Group* g = groups[i].PushArr(1);
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				GLASSERT(msc);
				g->bounds = msc->Bounds();
				FillGroup(g, pos, chit);
			}
		}
	}
}

void CircuitSim::DrawGroups()
{
	static float thickness = 1.0f / 32.0f;
	static float thicker = thickness * 2.0f;
	static float square = thickness * 4.0f;
	static float hSquare = square * 0.5f;
	static float gutter = thickness;
	static float half = thickness * 0.5f;
	static float arc = 1.0f / 16.0f;

	for (int i = 0; i < NUM_GROUPS; ++i) {
		canvas[i].Clear();
		for (const Group& group : groups[i]) {

			float x0 = float(group.bounds.min.x) + half + gutter;
			float y0 = float(group.bounds.min.y) + half + gutter;
			float x1 = float(group.bounds.max.x + 1) - half - gutter;
			float y1 = float(group.bounds.max.y + 1) - half - gutter;

			canvas[i].DrawRectangleOutline(x0, y0, x1 - x0, y1 - y0, thickness, arc);
		}
	}

	for (const Connection& c : connections) {
		// Groups come and go - need to find the group for the connection.
		Group* groupA = 0;
		Group* groupB = 0;
		int type = 0;
		if (ConnectionValid(c.a, c.b, &type, &groupA, &groupB)) {
			Vector2F p0, p1;
#if 0
			Rectangle2F rectA = ToWorld2F(groupA->bounds);
			Rectangle2F rectB = ToWorld2F(groupB->bounds);

			float len = FLT_MAX;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					Vector2F q0 = rectA.EdgeCenter(i);
					Vector2F q1 = rectB.EdgeCenter(j);
					float len2 = (q0 - q1).LengthSquared();
					if (len2 < len) {
						len = len2;
						p0 = q0;
						p1 = q1;
					}
				}
			}
#else
			p0 = ToWorld2F(c.a);
			p1 = ToWorld2F(c.b);
#endif
			canvas[type].DrawLine(p0.x, p0.y, p1.x, p1.y, thicker);
			canvas[type].DrawRectangle(p0.x - hSquare, p0.y - hSquare, square, square);
			canvas[type].DrawRectangle(p1.x - hSquare, p1.y - hSquare, square, square);
		}
	}
}


bool CircuitSim::ConnectionValid(const Vector2I& a, const Vector2I& b, int *type, Group** groupA, Group** groupB)
{
	int indexA = -1, typeA = NUM_GROUPS;
	int indexB = -1, typeB = NUM_GROUPS;
	if (FindGroup(a, &typeA, &indexA) && FindGroup(b, &typeB, &indexB)) {
		if (   (typeA == DEVICE_GROUP && typeB != DEVICE_GROUP)
			|| (typeA != DEVICE_GROUP && typeB == DEVICE_GROUP)) 
		{
			if (type) *type = (typeA == DEVICE_GROUP) ? typeB : typeA;
			if (groupA) *groupA = &groups[typeA][indexA];
			if (groupB) *groupB = &groups[typeB][indexB];
			return true;
		}
	}
	return false;
}


void CircuitSim::Connect(const grinliz::Vector2I& a, const grinliz::Vector2I& b)
{
	int type = 0;
	if (ConnectionValid(a, b, &type, 0, 0)) {
		// If power, filter out power connection.
		Connection c = { a, b, type };
		connections.Push(c);
	}
}


bool CircuitSim::FindGroup(const grinliz::Vector2I& pos, int* groupType, int* index)
{
	for (int j = 0; j < NUM_GROUPS; ++j) {
		for (int i = 0; i < groups[j].Size(); ++i) {
			if (groups[j][i].bounds.Contains(pos)) {
				if (groupType) *groupType = j;
				if (index) *index = i;
				return true;
			}
		}
	}
	return false;
}

