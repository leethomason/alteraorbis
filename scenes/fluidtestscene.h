#ifndef FLUID_TEST_SCENE_INCLUDED
#define FLUID_TEST_SCENE_INCLUDED

#include "../gamui/gamui.h"
#include "../xegame/scene.h"
#include "../xegame/cticker.h"

class LumosGame;
class WorldMap;


class FluidTestScene : public Scene
{
	typedef Scene super;
public:
	FluidTestScene(LumosGame* game);
	virtual ~FluidTestScene();

	virtual void Resize();
	void Zoom(int style, float delta);
	void Rotate(float degrees);
	void MoveCamera(float dx, float dy);

	virtual void Tap(int action, const grinliz::Vector2F& screen, const grinliz::Ray& world);
	virtual void ItemTapped(const gamui::UIItem* item);
	virtual void HandleHotKey(int mask) { super::HandleHotKey(mask); }

	virtual void Draw3D(U32 deltaTime);
	virtual void DrawDebugText();

	virtual void DoTick(U32 delta);

private:
	void Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world);

	enum {
		BUTTON_ROCK0,
		BUTTON_ROCK1,
		BUTTON_ROCK2,
		BUTTON_ROCK3,
		BUTTON_EMITTER,
		NUM_BUILD_BUTTONS
	};

	LumosGame* lumosGame;
	Engine*		engine;
	WorldMap*	worldMap;
	bool		settled;

	CTicker fluidTicker;

	gamui::PushButton okay;
	gamui::ToggleButton buildButton[NUM_BUILD_BUTTONS];
	gamui::PushButton saveButton, loadButton;

};


#endif // FLUID_TEST_SCENE_INCLUDED
