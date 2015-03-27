#include "fluidtestscene.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"
#include "../game/lumosgame.h"
#include "../game/lumosmath.h"
#include "../game/lumoschitbag.h"
#include "../game/team.h"
#include "../game/circuitsim.h"
#include "../engine/text.h"
#include "../game/fluidsim.h"
#include "../game/mapspatialcomponent.h"

using namespace gamui;
using namespace grinliz;


FluidTestScene::FluidTestScene(LumosGame* game) : Scene(game), fluidTicker(500)
{
	context.game = game;
	context.worldMap = new WorldMap(SECTOR_SIZE, SECTOR_SIZE);
	context.engine = new Engine(game->GetScreenportMutable(), game->GetDatabase(), context.worldMap);
	context.engine->LoadConfigFiles("./res/particles.xml", "./res/lighting.xml");

	context.worldMap->InitCircle();
	InitStd(&gamui2D, &okay, 0);

	context.chitBag = new LumosChitBag( context, 0 );
	context.worldMap->AttachEngine(context.engine, context.chitBag);

	context.circuitSim = new CircuitSim(&context);

	// FIXME: the first one sets the camera height and direction to something
	// reasonable, and the 2nd positions it. Weird behavior.
	context.engine->CameraLookAt(0, 3, 8, -45.f, -30.f);
	context.engine->CameraLookAt(float(SECTOR_SIZE / 2), float(SECTOR_SIZE / 2));

	static const char* NAME[NUM_BUTTONS] = { 
		"Rock0", "Rock1", "Rock2", "Rock3", "Water\nEmitter", "Lava\nEmitter", 
		"Temple", "Detector", "Switch On", "Switch Off", 
		"Gate", "Turret",
		"Delete", "Rotate" 
	};
	for (int i = 0; i < NUM_BUTTONS; ++i) {
		buildButton[i].Init(&gamui2D, game->GetButtonLook(0));
		buildButton[i].SetText(NAME[i]);
		buildButton[0].AddToToggleGroup(&buildButton[i]);
	}
	saveButton.Init(&gamui2D, game->GetButtonLook(0));
	saveButton.SetText("Save");
	loadButton.Init(&gamui2D, game->GetButtonLook(0));
	loadButton.SetText("Load");

	hover.Zero();
}


FluidTestScene::~FluidTestScene()
{
	delete context.circuitSim;
	context.worldMap->AttachEngine(0, 0);
	delete context.chitBag;
	delete context.worldMap;
	delete context.engine;
}


void FluidTestScene::Resize()
{
	PositionStd(&okay, 0);
	LayoutCalculator layout = DefaultLayout();

	int x = 0;
	int y = 0;
	for (int i = 0; i < NUM_BUTTONS; ++i) {
		layout.PosAbs(&buildButton[i], x, -3+y);
		++x;
		if (x == 10) {
			x = 0;
			++y;
		}
	}
	layout.PosAbs(&saveButton, 1, -1);
	layout.PosAbs(&loadButton, 2, -1);
}


void FluidTestScene::Zoom(int style, float delta)
{
	if (style == GAME_ZOOM_PINCH)
		context.engine->SetZoom(context.engine->GetZoom() *(1.0f + delta));
	else
		context.engine->SetZoom(context.engine->GetZoom() + delta);

}

void FluidTestScene::Rotate(float degrees)
{
	context.engine->camera.Orbit(degrees);
}


void FluidTestScene::MouseMove(const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Vector3F at = { 0, 0, 0 };
	float t = 0;
	IntersectRayAAPlane(world.origin, world.direction, 1, 0, &at, &t);
	hover.Set((int)at.x, (int)at.z);
}


void FluidTestScene::Tap(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	bool uiHasTap = ProcessTap(action, view, world);
	if (!uiHasTap) {
		Vector3F at = { 0, 0, 0 };
		float t = 0;
		int result = IntersectRayAAPlane(world.origin, world.direction, 1, 0, &at, &t);
		Process3DTap(action, view, world, context.engine);

		if (action == GAME_TAP_DOWN) {
			dragStart = ToWorld2I(at);
		}
		else if (action == GAME_TAP_UP) {
			Vector2I dragEnd = ToWorld2I(at);
			if (dragStart != dragEnd) {
				context.circuitSim->Connect(dragStart, dragEnd);
			}
			else {
				Tap3D(view, world);
			}
		}
	}
}


void FluidTestScene::Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Vector3F at = { 0, 0, 0 };
	float t = 0;
	int result = IntersectRayAAPlane(world.origin, world.direction, 1, 0, &at, &t);
	if (result == INTERSECT) {
		Vector2I pos2i = ToWorld2I(at);
		if (context.worldMap->Bounds().Contains(pos2i)) {

			bool trigger = false;
			if (!buildButton[BUTTON_DELETE].Down() && !buildButton[BUTTON_ROTATE].Down()) {
				Chit* building = context.chitBag->QueryBuilding(IString(), pos2i, 0);
				if (building) {
					context.circuitSim->TriggerDetector(pos2i);
					trigger = true;
				}
			}

			int id = -1;
			if (!trigger) {
				for (int i = 0; i < NUM_BUTTONS; ++i) {
					if (buildButton[i].Down()) {
						id = i;
						break;
					}
				}
			}
			if (id >= 0) {
				Chit* chit = 0;
				switch (id) {
					case BUTTON_ROCK0:
					case BUTTON_ROCK1:
					case BUTTON_ROCK2:
					case BUTTON_ROCK3:
					context.worldMap->SetRock(pos2i.x, pos2i.y, id - BUTTON_ROCK0, false, WorldGrid::ROCK);
					break;

					case BUTTON_EMITTER:
					context.worldMap->SetEmitter(pos2i.x, pos2i.y, true, WorldGrid::FLUID_WATER);
					break;

					case BUTTON_LAVA_EMITTER:
					context.worldMap->SetEmitter(pos2i.x, pos2i.y, true, WorldGrid::FLUID_LAVA);
					break;

					case BUTTON_SWITCH_ON:
					chit = context.chitBag->NewBuilding(pos2i, "switchOn", TEAM_HOUSE);
					break;

					case BUTTON_SWITCH_OFF:
					chit = context.chitBag->NewBuilding(pos2i, "switchOn", TEAM_HOUSE);
					break;

					case BUTTON_TEMPLE:
					chit = context.chitBag->NewBuilding(pos2i, "temple", TEAM_HOUSE);
					break;

					case BUTTON_GATE:
					chit = context.chitBag->NewBuilding(pos2i, "gate", TEAM_HOUSE);
					break;

					case BUTTON_DETECTOR:
					chit = context.chitBag->NewBuilding(pos2i, "detector", TEAM_HOUSE);
					break;

					case BUTTON_TURRET:
					chit = context.chitBag->NewBuilding(pos2i, "turret", TEAM_HOUSE);
					break;

					case BUTTON_DELETE:
					{
						Chit* building = context.chitBag->QueryBuilding(IString(), pos2i, 0);
						if (building) {
							building->QueueDelete();
						}
					}
					break;

					case BUTTON_ROTATE:
					{
						Chit* building = context.chitBag->QueryBuilding(IString(), pos2i, 0);
						if (building) {
							Matrix4 m;
							building->Rotation().ToMatrix(&m);
							float r = m.CalcRotationAroundAxis(1);
							r = NormalizeAngleDegrees(r + 90.0f);
							building->SetRotation(Quaternion::MakeYRotation(r));
						}

					}
					break;
				}
				if (chit) {
					MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
					if (msc) msc->SetNeedsCorePower(false);
				}
			}
		}
	}
}



void FluidTestScene::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		context.game->PopScene();
	}
	else if (item == &saveButton) {
		context.worldMap->Save("fluidtest.dat");
	}
	else if (item == &loadButton) {
		context.worldMap->Load("fluidtest.dat");
	}
}


void FluidTestScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, context.engine);
}


void FluidTestScene::Draw3D(U32 deltaTime)
{
	context.engine->Draw(deltaTime);
}


void FluidTestScene::DrawDebugText()
{
	static const int x = 0;
	int y = 120;
	DrawDebugTextDrawCalls(x, y, context.engine);
	Vector2I sector = { 0, 0 };
	UFOText::Instance()->Draw(x, y + 16, "Settled: %s voxel=%d,%d", context.worldMap->GetFluidSim(sector)->Settled() ? "true" : "false", hover.x, hover.y);
}


void FluidTestScene::DoTick(U32 delta)
{
	context.worldMap->DoTick(delta, context.chitBag);
	context.circuitSim->DoTick(delta);

	static const Vector2I home = { 0, 0 };
	context.circuitSim->CalcGroups(home);
	context.circuitSim->DrawGroups();

	context.chitBag->DoTick(delta);
}
