#include "fluidtestscene.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"
#include "../game/lumosgame.h"
#include "../game/lumosmath.h"
#include "../engine/text.h"

using namespace gamui;
using namespace grinliz;


FluidTestScene::FluidTestScene(LumosGame* game) : Scene(game), lumosGame(game), fluidTicker(500), settled(false)
{
	worldMap = new WorldMap(SECTOR_SIZE, SECTOR_SIZE);
	engine = new Engine(game->GetScreenportMutable(), game->GetDatabase(), worldMap);
	engine->LoadConfigFiles("./res/particles.xml", "./res/lighting.xml");

	worldMap->InitCircle();

	lumosGame->InitStd(&gamui2D, &okay, 0);

	// FIXME: the first one sets the camera height and direction to something
	// reasonable, and the 2nd positions it. Weird behavior.
	engine->CameraLookAt(0, 3, 8, -45.f, -30.f);
	engine->CameraLookAt(float(SECTOR_SIZE / 2), float(SECTOR_SIZE / 2));

	static const char* NAME[NUM_BUILD_BUTTONS] = { "Rock0", "Rock1", "Rock2", "Rock3", "Emitter" };
	for (int i = 0; i < NUM_BUILD_BUTTONS; ++i) {
		buildButton[i].Init(&gamui2D, game->GetButtonLook(0));
		buildButton[i].SetText(NAME[i]);
		buildButton[0].AddToToggleGroup(&buildButton[i]);
	}
	saveButton.Init(&gamui2D, game->GetButtonLook(0));
	saveButton.SetText("Save");
	loadButton.Init(&gamui2D, game->GetButtonLook(0));
	loadButton.SetText("Load");
}


FluidTestScene::~FluidTestScene()
{
	delete worldMap;
	delete engine;
}


void FluidTestScene::Resize()
{
	lumosGame->PositionStd(&okay, 0);
	LayoutCalculator layout = lumosGame->DefaultLayout();
	for (int i = 0; i < NUM_BUILD_BUTTONS; ++i) {
		layout.PosAbs(&buildButton[i], i, -2);
	}
	layout.PosAbs(&saveButton, 1, -1);
	layout.PosAbs(&loadButton, 2, -1);
}


void FluidTestScene::Zoom(int style, float delta)
{
	if (style == GAME_ZOOM_PINCH)
		engine->SetZoom(engine->GetZoom() *(1.0f + delta));
	else
		engine->SetZoom(engine->GetZoom() + delta);

}

void FluidTestScene::Rotate(float degrees)
{
	engine->camera.Orbit(degrees);
}


void FluidTestScene::Tap(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	bool uiHasTap = ProcessTap(action, view, world);
	if (!uiHasTap) {
		Process3DTap(action, view, world, engine);
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
		if (worldMap->Bounds().Contains(pos2i)) {
			if (buildButton[BUTTON_ROCK0].Down()) {
				worldMap->SetRock(pos2i.x, pos2i.y, 0, false, WorldGrid::ROCK);
			}
			else if (buildButton[BUTTON_ROCK1].Down()) {
				worldMap->SetRock(pos2i.x, pos2i.y, 1, false, WorldGrid::ROCK);
			}
			else if (buildButton[BUTTON_ROCK2].Down()) {
				worldMap->SetRock(pos2i.x, pos2i.y, 2, false, WorldGrid::ROCK);
			}
			else if (buildButton[BUTTON_ROCK3].Down()) {
				worldMap->SetRock(pos2i.x, pos2i.y, 3, false, WorldGrid::ROCK);
			}
			else if (buildButton[BUTTON_EMITTER].Down()) {
				worldMap->SetEmitter(pos2i.x, pos2i.y, true);
			}
		}
	}
}


void FluidTestScene::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		lumosGame->PopScene();
	}
	else if (item == &saveButton) {
		worldMap->Save("fluidtest.dat");
	}
	else if (item == &loadButton) {
		worldMap->Load("fluidtest.dat");
	}
}


void FluidTestScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, engine);
}


void FluidTestScene::Draw3D(U32 deltaTime)
{
	engine->Draw(deltaTime);
}


void FluidTestScene::DrawDebugText()
{
	static const int x = 0;
	int y = 120;
	DrawDebugTextDrawCalls(x, y, engine);
	UFOText::Instance()->Draw(x, y + 16, "Settled: %s", settled ? "true" : "false");
}


void FluidTestScene::DoTick(U32 delta)
{
	Vector2I sector = { 0, 0 };
	if (fluidTicker.Delta(delta)) {
		settled = worldMap->RunFluidSim(sector);
	}
	worldMap->EmitFluidParticles(delta, sector, engine);
}
