#include "../grinliz/glstringutil.h"

#include "gamescenemenu.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../game/aicomponent.h"
#include "../game/sim.h"
#include "../script/corescript.h"

using namespace gamui;
using namespace grinliz;

GameSceneMenu::GameSceneMenu(Gamui* gamui2D, LumosGame* game)
{
	useBuilding = false;
	canTeleport = false;

	cameraHomeButton.Init(gamui2D, game->GetButtonLook(0));
	cameraHomeButton.SetText( "Home" );
	cameraHomeButton.SetVisible( false );

	useBuildingButton.Init(gamui2D, game->GetButtonLook(0));
	useBuildingButton.SetText( "Use" );
	useBuildingButton.SetVisible( false );

	nextUnit.Init( gamui2D, game->GetButtonLook( 0 ));
	nextUnit.SetText( ">" );
	nextUnit.SetVisible( false );

	prevUnit.Init( gamui2D, game->GetButtonLook( 0 ));
	prevUnit.SetText( "<" );
	prevUnit.SetVisible( false );

	avatarUnit.Init(gamui2D, game->GetButtonLook(0));
	avatarUnit.SetText("Avatar");
	avatarUnit.SetVisible(false);

	static const char* modeButtonText[NUM_BUILD_CATEGORIES] = {
		"Utility", "Denizen", "Agronomy", "Economy", "Visitor", "Circuits"
	};
	for( int i=0; i<NUM_BUILD_CATEGORIES; ++i ) {
		modeButton[i].Init( gamui2D, game->GetButtonLook(0) );
		modeButton[i].SetText( modeButtonText[i] );
		modeButton[0].AddToToggleGroup( &modeButton[i] );
	}

	BuildScript	buildScript;
	for( int i=0; i<BuildScript::NUM_PLAYER_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );

		buildButton[i].Init( gamui2D, game->GetButtonLook(0) );
		buildButton[i].SetText( bd.label.safe_str() );

		if (bd.zone == BuildData::ZONE_INDUSTRIAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("anvil", true), game->CalcUIIconAtom("anvil", false));
		else if (bd.zone == BuildData::ZONE_NATURAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("leaf", true), game->CalcUIIconAtom("leaf", false));

		buildButton[0].AddToToggleGroup( &buildButton[i] );
		modeButton[bd.group].AddSubItem( &buildButton[i] );
	}
	buildButton[0].SetText("UNUSED");

	tabBar0.Init( gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );
	tabBar1.Init( gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );

	createWorkerButton.Init( gamui2D, game->GetButtonLook(0) );
	createWorkerButton.SetText( "WorkerBot" );
	nWorkers = -1;
	SetNumWorkers(0);

	buildDescription.Init(gamui2D);

	for( int i=0; i<NUM_UI_MODES; ++i ) {
		static const char* TEXT[NUM_UI_MODES] = { "View", "Build", "Control" };
		uiMode[i].Init( gamui2D, game->GetButtonLook(0));
		uiMode[i].SetText( TEXT[i] );
		uiMode[0].AddToToggleGroup( &uiMode[i] );
	}

	RenderAtom darkPurple = LumosGame::CalcPaletteAtom( 10, 5 );
	darkPurple.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	uiBackground.Init(gamui2D, darkPurple, false);

	uiMode[UI_VIEW].SetDown();

	for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
		static const char* NAMES[NUM_SQUAD_BUTTONS] = { "Local", "Alpha", "Beta", "Delta", "Omega" };
		squadButton[i].Init(gamui2D, game->GetButtonLook(0));
		squadButton[i].SetText(NAMES[i]);
		squadButton[0].AddToToggleGroup(&squadButton[i]);
	}
	for (int i = 0; i < MAX_CITIZENS; ++i) {
		squadBar[i].Init(gamui2D);
		squadBar[i].hitBounds.SetCapturesTap(true);
	}

	// Fake a tap to initialize.
	ItemTapped(&uiMode[UI_VIEW]);
}


void GameSceneMenu::Resize(const Screenport& port, const gamui::LayoutCalculator& layout)
{
	layout.PosAbs(&useBuildingButton, 0, 2);
	layout.PosAbs(&prevUnit, 1, 1);
	layout.PosAbs(&avatarUnit, 2, 1);
	layout.PosAbs(&nextUnit, 3, 1);
	layout.PosAbs(&cameraHomeButton, 0, 1);

	for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
		layout.PosAbs(&squadButton[i], i, 1);
	}

	{
		BuildScript buildScript;
		int level = BuildScript::GROUP_UTILITY;
		int start = 0;
		for (int i = 1; i < BuildScript::NUM_PLAYER_OPTIONS; ++i) {
			const BuildData& bd = buildScript.GetData(i);
			if (bd.group != level) {
				level = bd.group;
				start = i;
			}
			if (bd.group == 0) {
				layout.PosAbs(&buildButton[i], i - start - 1, 2);
			}
			else {
				int ld = i - start;
				int lx = ld % 6;
				int ly = ld / 6;
				layout.PosAbs(&buildButton[i], lx, ly + 2);
			}
		}
	}
//	buildButton[0].SetVisible(false);	// the "none" button. Not working - perhaps bug in sub-selection.
	buildButton[0].SetPos(-100, -100);

	for (int i = 0; i<NUM_BUILD_CATEGORIES; ++i) {
		layout.PosAbs( &modeButton[i], i, 1 );
	}
	layout.PosAbs(&createWorkerButton, 0, 3);

	for( int i=0; i<NUM_UI_MODES; ++i ) {
		layout.PosAbs( &uiMode[i], i, 0 );
	}

	layout.PosAbs(&buildDescription, 0, 4);
	tabBar0.SetPos(  uiMode[0].X(), uiMode[0].Y() );
	tabBar0.SetSize( uiMode[NUM_UI_MODES-1].X() + uiMode[NUM_UI_MODES-1].Width() - uiMode[0].X(), uiMode[0].Height() );
	tabBar1.SetPos(  modeButton[0].X(), modeButton[0].Y() );
	tabBar1.SetSize( modeButton[NUM_BUILD_CATEGORIES-1].X() + modeButton[NUM_BUILD_CATEGORIES-1].Width() - modeButton[0].X(), modeButton[0].Height() );

	// ------- SQUAD LAYOUT ------ //
	LayoutCalculator layout2 = layout;
	layout2.SetSize(layout2.Width(), layout2.Height()*0.5f);
	for (int j = 0; j < CITIZEN_BASE; ++j) {
		layout2.PosAbs(&squadBar[j], 0, 2*2 + j);
	}
	for (int i = 0; i < MAX_SQUADS; ++i) {
		for (int j = 0; j < SQUAD_SIZE; ++j) {
			layout2.PosAbs(&squadBar[CITIZEN_BASE + SQUAD_SIZE*i + j], i+1, 2*2 + j);
		}
	}

	// --- calculated ----
	uiBackground.SetPos(squadBar[0].X(), squadBar[0].Y());
	uiBackground.SetSize(squadBar[MAX_CITIZENS - 1].X() + squadBar[MAX_CITIZENS - 1].Width() - squadBar[0].X(),
						 squadBar[CITIZEN_BASE - 1].Y() + squadBar[CITIZEN_BASE - 1].Height() - squadBar[0].Y());
}


void GameSceneMenu::DoTick(CoreScript* cs, const int* nBuilding, int nBuildingCount)
{
	int uiMode = UIMode();
	if (uiMode == UI_CONTROL) {
		SetSquadDisplay(cs);
	}


	BuildScript	buildScript;
	CStr<64> str;
	for (int i = 0; i < BuildScript::NUM_PLAYER_OPTIONS && i < nBuildingCount; ++i) {
		const BuildData& bd = buildScript.GetData(i);
		bd.LabelWithCount(nBuilding[i], &str);
		buildButton[i].SetText(str.safe_str());
	}
}


void GameSceneMenu::ItemTapped(const gamui::UIItem* item)
{
	// Only process menu logic; not game effects.
	// Check high level buttons first:
	for (int i = 0; i < NUM_UI_MODES; ++i) {
		if (item == &uiMode[i]) {
			int uiMode = UIMode();

			cameraHomeButton.SetVisible(uiMode == UI_VIEW);
			prevUnit.SetVisible(uiMode == UI_VIEW);
			avatarUnit.SetVisible(uiMode == UI_VIEW);
			nextUnit.SetVisible(uiMode == UI_VIEW);
			useBuildingButton.SetVisible(uiMode == UI_VIEW && useBuilding);
			tabBar0.SetVisible(false);
			tabBar1.SetVisible(false);
			createWorkerButton.SetVisible(uiMode == UI_BUILD);
			buildDescription.SetVisible(uiMode == UI_BUILD);

			for (int i = 0; i < NUM_BUILD_CATEGORIES; ++i) {
				modeButton[i].SetVisible(uiMode == UI_BUILD);
			}
			// FIXME: set the build buttons
			for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
				squadButton[i].SetVisible(uiMode == UI_CONTROL);
			}
			for (int i = 0; i < MAX_CITIZENS; ++i) {
				squadBar[i].SetVisible(false);	// turned on in the SetSquadDisplay()
			}
			uiBackground.SetVisible(uiMode == UI_CONTROL);

			// Any mode change clears the current build:
			buildButton[0].SetDown();
			break;
		}
	}
	// Any category change clears the current build:
	for (int i = 0; i < NUM_BUILD_CATEGORIES; ++i) {
		if (item == &modeButton[i]) {
			buildButton[0].SetDown();
			break;
		}
	}

	if (uiMode[UI_BUILD].Down()) {
		// Toggle the build based on circuits (since they have 2 rows.)
		createWorkerButton.SetVisible(!modeButton[NUM_BUILD_CATEGORIES - 1].Down());

		// Set the build description.
		int buildActive = BuildActive();
		if (buildActive == 0) {
			buildDescription.SetText("Tap and drag buildings to rotate them.");
		}
		else {
			BuildScript buildScript;
			const BuildData& bd = buildScript.GetData(buildActive);
			buildDescription.SetText(bd.desc ? bd.desc : "");
		}
	}
	else {
		buildDescription.SetText("");
	}
}


int GameSceneMenu::UIMode() const
{
	for (int i = 0; i < NUM_UI_MODES; ++i) {
		if (uiMode[i].Down()) {
			return i;
		}
	}
	return NUM_UI_MODES;
}


void GameSceneMenu::EnableBuildAndControl(bool enable)
{
	if (!enable && UIMode() != UI_VIEW) {
		DoEscape(true);
	}
	uiMode[UI_BUILD].SetEnabled(enable);
	uiMode[UI_CONTROL].SetEnabled(enable);
}


void GameSceneMenu::SetNumWorkers(int n)
{
	if (n != nWorkers) {
		nWorkers = n;

		CStr<32> str;
		str.Format("WorkerBot\n%d %d/%d", WORKER_BOT_COST, nWorkers, MAX_WORKER_BOTS);
		createWorkerButton.SetText( str.c_str() );
		createWorkerButton.SetEnabled( nWorkers < MAX_WORKER_BOTS );
	}
}


void GameSceneMenu::SetUseBuilding(bool canUse)
{
	if (canUse != useBuilding) {
		useBuilding = canUse;
		useBuildingButton.SetVisible(UIMode() == UI_VIEW && useBuilding);
	}
}


void GameSceneMenu::SetCanTeleport(bool t)
{
	if (t != canTeleport) {
		canTeleport = t; 
		avatarUnit.SetText(canTeleport ? "Teleport\nAvatar" : "Avatar");
	}
}


bool GameSceneMenu::CircuitMode() const
{
	return (   UIMode() == UI_BUILD
			&& modeButton[BUILD_CATEGORY_CIRCUIT].Down());
}


int GameSceneMenu::BuildActive() const
{
	if (uiMode[UI_BUILD].Down()) {
		for (int i = 1; i < BuildScript::NUM_PLAYER_OPTIONS; ++i) {
			if (buildButton[i].Down()) {
				return i;
			}
		}
	}
	return 0;
}


void GameSceneMenu::SetSquadDisplay(CoreScript* cs)
{
	if (!cs) return;
	bool inUse[MAX_CITIZENS] = { false };

	// -- Set the bar for individual citizens --
	int count[NUM_SQUAD_BUTTONS] = { 0 };
	GLString str;
	CChitArray citizens;
	cs->Citizens(&citizens);

	for (int i = 0; i < citizens.Size(); ++i) {
		int c = cs->SquadID(citizens[i]->ID()) + 1;
		GLASSERT(c >= 0 && c < NUM_SQUAD_BUTTONS);
		if (count[c] < ((c == 0) ? CITIZEN_BASE : SQUAD_SIZE)) {
//			const GameItem* item = citizens[i]->GetItem();
			int index = 0;
			if (c == 0) {
				index = count[0];
				GLASSERT(index >= 0 && index < CITIZEN_BASE);
			}
			else {
				index = CITIZEN_BASE + (c - 1)*SQUAD_SIZE + count[c];
				GLASSERT(index >= CITIZEN_BASE && index < MAX_CITIZENS);
			}
			count[c] += 1;
			GLASSERT(index >= 0 && index < MAX_CITIZENS);
			ItemComponent* itemComponent = citizens[i]->GetItemComponent();
			squadBar[index].Set(itemComponent);
			squadBar[index].SetVisible(true);
			squadBar[index].userItemID = citizens[i]->ID();
			inUse[index] = true;
		}
	}

	for (int i = 0; i < MAX_CITIZENS; ++i) {
		if (!inUse[i]) {
			squadBar[i].SetVisible(false);
			squadBar[i].userItemID = 0;
		}
	}

	// -- Put up information about the squad itself -- //
	for (int i = 0; i < MAX_SQUADS; ++i) {
		static const char* NAME[MAX_SQUADS] = { "Alpha", "Beta", "Delta", "Omega" };
		// Ready, Resting, On Route
		Vector2I waypoint = { 0, 0 };
		Vector2I sector = { 0, 0 };
		CChitArray squaddies;
		if (cs) {
			waypoint = cs->GetWaypoint(i);
			cs->Squaddies(i, &squaddies);
		}
		double totalMorale = 0;
		for (int k = 0; k < squaddies.Size(); ++k) {
			if (squaddies[k]->GetAIComponent()) {
				totalMorale += squaddies[k]->GetAIComponent()->GetNeeds().Morale();
			}
			if (sector.IsZero()) {
				sector = ToSector(squaddies[k]->Position());
			}
		}
		double moraleAve = squaddies.Size() ? (totalMorale / double(squaddies.Size())) : 0;

		CStr<32> str = NAME[i];
		if (squaddies.Size() && !waypoint.IsZero()) {
			str.Format("%s\nRoute %c%d", NAME[i], 'A' + (waypoint.x / SECTOR_SIZE), 1 + (waypoint.y / SECTOR_SIZE));
		}
		else if (squaddies.Size() && !sector.IsZero() && sector != ToSector(cs->ParentChit()->Position())) {
			str.Format("%s\nAt %c%d", NAME[i], 'A' + sector.x, 1 + sector.y);
		}
		else if (squaddies.Size() && moraleAve > 0.95) {
			str.Format("%s\nReady", NAME[i]);
		}
		else if (squaddies.Size()) {
			str.Format("%s\nRest %d%%", NAME[i], int(moraleAve*100.0f));
		}
		squadButton[i + 1].SetText(str.safe_str());
	}
}



void GameSceneMenu::DoEscape(bool fullEscape)
{
	int mode = UIMode();
	if (mode == UI_VIEW) {
		// no sub-options. do nothing.
	}
	else if (mode == UI_CONTROL) {
		// Back to view.
		uiMode[UI_VIEW].SetDown();
		ItemTapped(&uiMode[UI_VIEW]);
	}
	else if (mode == UI_BUILD) {
		// How far down are we?
		int buildActive = BuildActive();
		if (buildActive) {
			buildButton[0].SetDown();	// the no-op button. something has to be down.
			buildDescription.SetText("Tap and drag buildings to rotate them.");
		}
		else {
			uiMode[UI_VIEW].SetDown();
			ItemTapped(&uiMode[UI_VIEW]);
		}
	}
}
