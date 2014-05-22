#include "plantscript.h"
#include "itemscript.h"

#include "../engine/serialize.h"
#include "../engine/engine.h"

#include "../xegame/chitbag.h"

#include "../grinliz/glstringutil.h"

#include "../game/gamelimits.h"
#include "../game/weather.h"
#include "../game/worldmap.h"
#include "../game/sim.h"
#include "../game//lumoschitbag.h"

using namespace grinliz;

static const int	TIME_TO_GROW  = 4 * (1000 * 60);	// minutes
static const int	TIME_TO_SPORE = 3 * (1000 * 60); 
static const float	HP_PER_SECOND = 1.0f;

const GameItem* PlantScript::plantDef[NUM_PLANT_TYPES];

const GameItem* PlantScript::PlantDef(int plant0Based)
{
	GLASSERT(plant0Based >= 0 && plant0Based < NUM_PLANT_TYPES);
	if (plantDef[0] == 0) {
		for (int i = 0; i < NUM_PLANT_TYPES; ++i) {
			CStr<32> str;
			str.Format("plant%d", i);
			plantDef[i] = &ItemDefDB::Instance()->Get(str.c_str());
		}
	}
	return plantDef[plant0Based];
}


PlantScript::PlantScript(const ChitContext* c) : index(0), context(c)
{
	random.SetSeedFromTime();
}


void PlantScript::DoTick(U32 delta)
{
	// We need process at a steady rate so that
	// the time between ticks is constant.
	// This is performance regressive, so something
	// to keep an eye on.
	static const int MAP2 = MAX_MAP_SIZE*MAX_MAP_SIZE;
	static const int DELTA = 2000;	// How frequenty to tick a given plant
	static const int N_PER_MSEC = MAP2 / DELTA;
	static const int PRIME = 1553;

	static const float SHADE_EFFECT = 0.7f;

	int n = N_PER_MSEC * delta;
	Weather* weather = Weather::Instance();

	WorldMap* worldMap = context->worldMap;
	Rectangle2I bounds = worldMap->Bounds();
	bounds.Outset(-1);	// edge of map: don't want to tap over the edge.

	const Vector3F& light = context->engine->lighting.direction;
	const float		norm = Max(fabs(light.x), fabs(light.z));
	Vector2I		lightTap = { LRintf(light.x / norm), LRintf(light.z / norm) };

	for (int i = 0; i < n; ++i) {
		index += PRIME;

		GLASSERT(MAX_MAP_SIZE == 1024);
		int x = index & 1023;
		int y = (index >> 10) & 1023;

		const WorldGrid& wg = worldMap->GetWorldGrid(x, y);
		if (!wg.Plant()) continue;

		Vector2I pos2i = { x, y };
		Vector2F pos2f = ToWorld2F(pos2i);

		// --- Light Tap --- //
		const float	height			= (float)(wg.PlantStage() + 1) * 0.5f;	// plant height about 1/2 rock height
		const float	rainBase		= weather->RainFraction(pos2f.x, pos2f.y);
		const float sunBase			= (1.0f - rainBase);
		const float temperatureBase	= weather->Temperature(pos2f.x, pos2f.y);

		float rain			= rainBase;
		float sun			= sunBase;
		float temperature	= temperatureBase;

		Vector2I tap = pos2i + lightTap;

		// Check for something between us and the light.
		const WorldGrid& wgTap = worldMap->GetWorldGrid(tap);
		float tapHeight = float(wgTap.RockHeight());
		if (wgTap.PlantStage()) {
			tapHeight = float(wgTap.PlantStage() + 1) * 0.5f;	// plant height is about 1/2 the rock height
		}

		if (tapHeight > (height + 0.1f)) {
			// in shade
			sun *= SHADE_EFFECT;
			temperature *= SHADE_EFFECT;
		}

		// ---- Adjacent --- //
		static const Vector2I check[4] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
		for (int i = 0; i<GL_C_ARRAY_SIZE(check); ++i) {
			tap = pos2i + check[i];
			const WorldGrid& wgAdj = worldMap->GetWorldGrid(tap.x, tap.y);

			if (wgAdj.RockHeight()) {
				// Water or rock runoff increase water.
				rain += 0.25f * rainBase;
			}
			if (wgAdj.IsWater()) {
				rain += 0.25f;	// more water
				temperature = 0.50f * temperature + 0.50f * temperatureBase;	// moderate temperature
			}
		}
		rain = Clamp(rain, 0.0f, 1.0f);
		temperature = Clamp(temperature, 0.0f, 1.0f);
		sun = Clamp(sun, 0.0f, 1.0f);

		// ------- calc ------- //
		Vector3F actual = { sun, rain, temperature };
		Vector3F optimal = { 0.5f, 0.5f, 0.5f };

		const GameItem* item = PlantScript::PlantDef(wg.Plant() - 1);
		item->keyValues.Get(ISC::sun, &optimal.x);
		item->keyValues.Get(ISC::rain, &optimal.y);
		item->keyValues.Get(ISC::temp, &optimal.z);

		float distance = (optimal - actual).Length();

		const float GROW = Lerp(0.2f, 0.1f, (float)wg.PlantStage() / (float)(MAX_PLANT_STAGES - 1));
		const float DIE = 0.4f;
		float seconds = float(DELTA) / 1000.0f;

		if (distance < GROW) {
			// Heal.
			float hp = HP_PER_SECOND*seconds;
			DamageDesc heal( -hp, 0 );
			worldMap->VoxelHit(pos2i, heal);

			// Grow
			int nStage = wg.Plant() < 7 ? 4 : 2;

			if (wg.HPFraction() > 0.9f) {
				if (wg.PlantStage() < (nStage - 1)) {
					int hp = wg.HP();
					worldMap->SetPlant(pos2i.x, pos2i.y, wg.Plant(), wg.PlantStage() + 1);
					worldMap->SetWorldGridHP(pos2i.x, pos2i.y, hp);
				}
				if (random.Rand(32) < (U32)wg.PlantStage() ) {
					// Number range reflects wind direction.
					int dx = -1 + random.Rand(4);	// [-1,2]
					int dy = -1 + random.Rand(3);	// [-1,1]

					// Remember that create plant will favor creating
					// existing plants, so we don't need to specify
					// what to create.
					Sim* sim = context->chitBag->GetSim();
					GLASSERT(sim);
					sim->CreatePlant(pos2i.x + dx, pos2i.y + dy, -1);
				}
			}
		}
		else if (distance > DIE) {
			DamageDesc dd(HP_PER_SECOND * seconds, 0);
			worldMap->VoxelHit(pos2i, dd);
			if (wg.HP() == 0) {
				worldMap->SetPlant(pos2i.x, pos2i.y, 0, 0);
			}
		}
	}
}

