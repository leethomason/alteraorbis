#include "fluidtestscene.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"
#include "../game/lumosgame.h"
#include "../game/lumosmath.h"
#include "../game/lumoschitbag.h"
#include "../game/team.h"
#include "../game/circuitsim.h"
#include "../engine/text.h"

using namespace gamui;
using namespace grinliz;


FluidTestScene::FluidTestScene(LumosGame* game) : Scene(game), fluidTicker(500), settled(false)
{
	context.game = game;
	context.worldMap = new WorldMap(SECTOR_SIZE, SECTOR_SIZE);
	context.engine = new Engine(game->GetScreenportMutable(), game->GetDatabase(), context.worldMap);
	context.engine->LoadConfigFiles("./res/particles.xml", "./res/lighting.xml");

	context.worldMap->InitCircle();
	context.game->InitStd(&gamui2D, &okay, 0);

	context.chitBag = new LumosChitBag( context, 0 );
	context.worldMap->AttachEngine(context.engine, context.chitBag);

	context.circuitSim = new CircuitSim(context.worldMap, context.engine, 0);

	// FIXME: the first one sets the camera height and direction to something
	// reasonable, and the 2nd positions it. Weird behavior.
	context.engine->CameraLookAt(0, 3, 8, -45.f, -30.f);
	context.engine->CameraLookAt(float(SECTOR_SIZE / 2), float(SECTOR_SIZE / 2));

	static const char* NAME[NUM_BUTTONS] = { "Rock0", "Rock1", "Rock2", "Rock3", "Water\nEmitter", "Lava\nEmitter", "Green", "Violet", "Mantis",
											 "Switch", "Battery", "Zapper", "Bend", "Fork2", "Ice", "Stop", "Detect\nSmall", "Detect\nLarge", "Transistor", "Rotate" };
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
	context.game->PositionStd(&okay, 0);
	LayoutCalculator layout = context.game->DefaultLayout();
	for (int i = 0; i < NUM_BUTTONS; ++i) {
		if (i < NUM_BUILD_BUTTONS)
			layout.PosAbs(&buildButton[i], i, -3);
		else
			layout.PosAbs(&buildButton[i], i - NUM_BUILD_BUTTONS, -2);
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
		Process3DTap(action, view, world, context.engine);
		if (action == GAME_TAP_UP) {
			Tap3D(view, world);
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

			const WorldGrid& wg = context.worldMap->GetWorldGrid(pos2i);
			if (buildButton[BUTTON_ROTATE].Down()) {
				int rot = context.worldMap->CircuitRotation(pos2i.x, pos2i.y);
				context.worldMap->SetCircuitRotation(pos2i.x, pos2i.y, rot+1);
			}
			else if (wg.Circuit() == CIRCUIT_SWITCH) {
				context.circuitSim->TriggerSwitch(pos2i);
			}
			else if (wg.Circuit() == CIRCUIT_DETECT_ENEMY ) {
				context.circuitSim->TriggerDetector(pos2i);
			}
			else {
				for (int i = BUTTON_SWITCH; i < NUM_BUTTONS; ++i) {
					if (buildButton[i].Down()) {
						int circuit = i - BUTTON_SWITCH + 1;
						context.worldMap->SetCircuit(pos2i.x, pos2i.y, circuit);
					}
				}
				if (buildButton[BUTTON_ROCK0].Down()) {
					context.worldMap->SetRock(pos2i.x, pos2i.y, 0, false, WorldGrid::ROCK);
				}
				else if (buildButton[BUTTON_ROCK1].Down()) {
					context.worldMap->SetRock(pos2i.x, pos2i.y, 1, false, WorldGrid::ROCK);
				}
				else if (buildButton[BUTTON_ROCK2].Down()) {
					context.worldMap->SetRock(pos2i.x, pos2i.y, 2, false, WorldGrid::ROCK);
				}
				else if (buildButton[BUTTON_ROCK3].Down()) {
					context.worldMap->SetRock(pos2i.x, pos2i.y, 3, false, WorldGrid::ROCK);
				}
				else if (buildButton[BUTTON_EMITTER].Down()) {
					context.worldMap->SetEmitter(pos2i.x, pos2i.y, true, WorldGrid::FLUID_WATER);
				}
				else if (buildButton[BUTTON_LAVA_EMITTER].Down()) {
					context.worldMap->SetEmitter(pos2i.x, pos2i.y, true, WorldGrid::FLUID_LAVA);
				}
				else if (buildButton[BUTTON_GREEN].Down()) {
					context.chitBag->NewCrystalChit(at, CRYSTAL_GREEN, false);
				}
				else if (buildButton[BUTTON_VIOLET].Down()) {
					context.chitBag->NewCrystalChit(at, CRYSTAL_VIOLET, false);
				}
				else if (buildButton[BUTTON_MANTIS].Down()) {
					context.chitBag->NewMonsterChit(at, "mantis", TEAM_GREEN_MANTIS);
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
	UFOText::Instance()->Draw(x, y + 16, "Settled: %s voxel=%d,%d", settled ? "true" : "false", hover.x, hover.y);
}


void FluidTestScene::DoTick(U32 delta)
{
	context.worldMap->DoTick(delta, context.chitBag);
	context.circuitSim->DoTick(delta);
	context.chitBag->DoTick(delta);
}
