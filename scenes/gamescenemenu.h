#ifndef GAME_SCENE_MENU_INCLUDED
#define GAME_SCENE_MENU_INCLUDED

#include "../gamui/gamui.h"
#include "../script/buildscript.h"
#include "../game/gamelimits.h"
#include "../widget/hpbar.h"

class Screenport;
class LumosGame;
class Sim;
class CoreScript;

class GameSceneMenu
{
public:
	GameSceneMenu(gamui::Gamui* gamui2D, LumosGame* game);
	~GameSceneMenu()	{}

	void Resize(const Screenport& port, const gamui::LayoutCalculator& layout);
	void DoTick(CoreScript* coreScript);

	enum {
		NUM_SQUAD_BUTTONS = MAX_SQUADS + 1,	// "local" control, plus the squads
		NUM_BUILD_CATEGORIES = 6
	};

	enum {
		UI_VIEW,
		UI_BUILD,
		UI_CONTROL,
		NUM_UI_MODES
	};

	void ItemTapped(const gamui::UIItem* item);

	int UIMode() const;

	void EnableBuildAndControl(bool enable);
	void SetNumWorkers(int n);
	void SetUseBuilding(bool canUse);
	void SetCanTeleport(bool canTeleport);

	void DoEscape(bool fullEscape);
	int BuildActive() const;

	gamui::ToggleButton	uiMode[NUM_UI_MODES];

	gamui::PushButton	cameraHomeButton;
	gamui::PushButton	nextUnit, avatarUnit, prevUnit;
	gamui::PushButton	useBuildingButton;
	gamui::Image		tabBar0, tabBar1;
	gamui::PushButton	createWorkerButton;
	gamui::TextLabel	buildDescription;

	gamui::ToggleButton modeButton[NUM_BUILD_CATEGORIES];
	gamui::ToggleButton	buildButton[BuildScript::NUM_PLAYER_OPTIONS];
	gamui::ToggleButton	squadButton[NUM_SQUAD_BUTTONS];
	HPBar				squadBar[MAX_CITIZENS];
	gamui::Image		uiBackground;

private:
	void SetSquadDisplay(CoreScript* core);
	
	bool useBuilding;
	bool canTeleport;
	int nWorkers;
};

#endif // GAME_SCENE_MENU_INCLUDED
