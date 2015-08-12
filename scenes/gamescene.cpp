
#include <time.h>

#include "gamescene.h"
#include "characterscene.h"
#include "gamescenemenu.h"

#include "../version.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/cameracomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../engine/settings.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../game/sim.h"
#include "../game/pathmovecomponent.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/aicomponent.h"
#include "../game/reservebank.h"
#include "../game/workqueue.h"
#include "../game/mapspatialcomponent.h"
#include "../game/team.h"
#include "../game/adviser.h"
#include "../game/fluidsim.h"
#include "../game/gridmovecomponent.h"
#include "../game/physicssims.h"

#include "../engine/engine.h"
#include "../engine/text.h"

#include "../script/procedural.h"
#include "../script/corescript.h"
#include "../script/itemscript.h"
#include "../script/plantscript.h"

#include "../scenes/mapscene.h"
#include "../scenes/censusscene.h"

#include "../ai/rebuildai.h"
#include "../ai/domainai.h"

#include "../widget/tutorialwidget.h"

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 1.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	dragMode = EDragMode::NONE;
	targetChit = 0;
	possibleChit = 0;
	infoID = 0;
	chitTracking = 0;
	endTimer = 0;
	coreWarningTimer = 0;
	domainWarningTimer = 0;
	poolView = 0;
	paused = false;
	attached.Zero();
	voxelInfoID.Zero();
	mapDragStart.Zero();
	lumosGame = game;
	adviser = new Adviser();
	tutorial = new TutorialWidget();
	InitStd( &gamui2D, &okay, 0 );

	sim = new Sim( lumosGame );
	menu = new GameSceneMenu(&gamui2D, game);

	Load();
	
	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );
	minimap.SetCapturesTap( true );

	selectionTile.Init(&sim->Context()->worldMap->overlay1 , atom, true);

	atlasButton.Init(&gamui2D, game->GetButtonLook(0));
	atlasButton.SetText("Map");

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );

	saveButton.Init( &gamui2D, game->GetButtonLook(0) );
	saveButton.SetText( "Save" );

	pausedLabel.Init(&gamui2D);
	pausedLabel.SetText("-- Paused --");

	const RenderAtom nullAtom;
	helpText.Init(&gamui2D);
	helpImage.Init(&gamui2D, nullAtom, true);
	
	allRockButton.Init( &gamui2D, game->GetButtonLook(0) );
	allRockButton.SetText( "All Rock" );
	censusButton.Init( &gamui2D, game->GetButtonLook(0) );
	censusButton.SetText( "Census" );
	viewButton.Init(&gamui2D, game->GetButtonLook(0));
	viewButton.SetText("Camera");
	pauseButton.Init(&gamui2D, game->GetButtonLook(0));
	pauseButton.SetText("Pause");

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		newsButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		newsButton[i].SetSize( NEWS_BUTTON_WIDTH, NEWS_BUTTON_HEIGHT );
		newsButton[i].SetText( "news" );
	}

	abandonButton.Init(&gamui2D, game->GetButtonLook(0));
	abandonButton.SetText("Abandon\nDomain");
	abandonConfirmButton.Init(&gamui2D, game->GetButtonLook(0));
	abandonConfirmButton.SetText("Abandon\nConfirm");

	faceWidget.Init( &gamui2D, game->GetButtonLook(0), FaceWidget::ALL, 2 );
	faceWidget.SetSize( 100, 100 );

	targetFaceWidget.Init(&gamui2D, game->GetButtonLook(0), FaceWidget::BATTLE_BARS | FaceWidget::SHOW_NAME, 1);
	targetFaceWidget.SetSize(100, 100);

	summaryBars.Init(&gamui2D, 0, ai::Needs::NUM_NEEDS + 1);
	for (int i = 0; i < ai::Needs::NUM_NEEDS; ++i) {
		summaryBars.barArr[i+1]->SetText(ai::Needs::Name(i));
	}
	summaryBars.barArr[0]->SetText("morale");

	chitTracking = GetPlayerChitID();

#if 0
	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		pickupButton[i].Init( &gamui2D, game->GetButtonLook(0) );
	}
#endif

	RenderAtom grey  = LumosGame::CalcPaletteAtom( 0, 6 );

	dateLabel.Init( &gamui2D );
	techLabel.Init( &gamui2D );
	moneyWidget.Init( &gamui2D );
	newsConsole.Init( &gamui2D, sim->GetChitBag() );

	LayoutCalculator layout = DefaultLayout();
	startGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);
	endGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);

	Vector3F delta = { 14.0f, 14.0f, 14.0f };
	Vector3F target = { (float)sim->GetWorldMap()->Width() *0.5f, 0.0f, (float)sim->GetWorldMap()->Height() * 0.5f };
	if (GetPlayerChit()) {
		target = GetPlayerChit()->Position();
	}
	else if (GetHomeCore()) {
		target = GetHomeCore()->ParentChit()->Position();
	}
	sim->GetEngine()->CameraLookAt(target + delta, target);
	sim->GetChitBag()->AddListener(this);

	for (int i = 0; i < NUM_BUILD_MARKS; ++i) {
		buildMark[i].Init(&sim->GetWorldMap()->overlay1, grey, true);
		buildMark[i].SetVisible(false);
	}

	RenderAtom redAtom = LumosGame::CalcUIIconAtom("warning");
	RenderAtom yellowAtom = LumosGame::CalcUIIconAtom("yellowwarning");

	coreWarningIcon.Init(&gamui2D, game->GetButtonLook(0));
	domainWarningIcon.Init(&gamui2D, game->GetButtonLook(0));
	coreWarningIcon.SetDeco(redAtom, redAtom);
	domainWarningIcon.SetDeco(yellowAtom, yellowAtom);

	coreWarningIcon.SetText("WARNING: Core under attack.");
	domainWarningIcon.SetText("WARNING: Domain under attack.");

	adviser->Attach(&helpText, &helpImage);
	tutorial->Init(&gamui2D, game->GetButtonLook(0), layout);

	{
		tutorial->Add(0,
					  "Welcome to Altera, Domain Core. I am your Adviser. Mother Core has opened Her eyes and Her breath has "
					  "awoken our world. Many creatures, denizens, monsters, flora, and fauna seek to live or rule here. "
					  "Our goal is survive and prosper.");
		tutorial->Add(0, 
					  "As the domain core, it is your responsibility to construct buildings, defenses, manage the economy, "
					  "and direct our squads. You are given direct control of one unit: the Avatar.\n\n"
					  "The Avatar can explore and battle, but also craft weapons (at a Forge), use a Market, Exchange, "
					  "and Vault.\n\n"
					  "All the other denizens of your domain move of their own accord." );

		tutorial->Add(&menu->uiMode[GameSceneMenu::UI_AVATAR], 
					  "Avatar Mode allows you to control the Avatar, move the camera to the avatar or home core, and teleport the "
					  "Avatar home.");
		tutorial->Add(&menu->uiMode[GameSceneMenu::UI_VIEW],
					  "View Mode allows you follow your denizens, look around the world, and get information on "
					  "monsters, denizens, and buildings.");
		tutorial->Add(&menu->uiMode[GameSceneMenu::UI_BUILD], 
					  "In Build Mode you can construct buildings and defenses.");
		tutorial->Add(&menu->uiMode[GameSceneMenu::UI_CONTROL], 
					  "Once our domain is populous, Squad Mode gives an overview of our denizens, and allows you to "
					  "send squads on missions to attack and explore.");
		tutorial->Add(&dateLabel, 
					  "General information: the current date, the domain you are viewing, and the population (current/max) of our domain.");
		tutorial->Add(&minimap, 
					  "Tapping on the map lets you view anywhere in the world. The button below opens the map screen "
					  "to see a strategic view and the current Web.");
		tutorial->Add(&summaryBars, 
					  "Every unit (except the Avatar) has a need for food, energy, and fun provided by buildings of the domain. "
					  "Getting these needs met (or not), as well as other events effects the unit's morale. "
					  "Selecting a particular unit will show his/her/its need bar. "
					  "The average need and morale for your denizens is shown here. If a bar turns red, "
					  "one or more of your denizens has a critical need.\n\n"
					  "If you units morale goes to zero, they can delete or go mad." );
		tutorial->Add(&techLabel, 
					  "The current technology level of your domain. More tech has many advantages, including "
					  "more efficient buildings and better weapons. Visitors bring tech, and Temples and Kiosks bring visitors. "
					  "(I will advise you to build Temples and Kiosks when the time comes.)");
		tutorial->Add(&moneyWidget, 
					  "The wealth of our domain core. Au is used to purchase buildings. Crystal (green, red, blue, violet) "
					  "is used to construct items. Your Avatar contributes all the Au and Crystal he/she/it finds to the Core." );
		tutorial->Add(&censusButton, 
					  "The census provides a high level count of world population and notable monsters and items. "
					  "It can also be used as a research tool to find important or powerful items." );
		tutorial->Add(nullptr, 
					  "This world persists will automatically save when you exit.\n\n"
					  "Good luck. I will provide ongoing advice. Long may our domain stand.");
	}
	tutorial->ShowTutorial();
	tutorial->SetVisible(!sim->GetChitBag()->GetHomeCore());
}


GameScene::~GameScene()
{
	delete menu;
	delete sim;
	delete adviser;
	delete tutorial;
}


void GameScene::Resize()
{
	const Screenport& port = lumosGame->GetScreenport();
	LayoutCalculator layout = DefaultLayout();

	menu->Resize(port, layout);

	layout.PosAbs(&censusButton, 0, -1);
	layout.PosAbs(&helpImage, 1, -1);
	layout.PosAbs(&helpText, 2, -1);
	layout.PosAbs(&saveButton, 1, -1);
	layout.PosAbs(&allRockButton, 3, -1);
	layout.PosAbs(&okay, 4, -1);

	layout.PosAbs(&viewButton, -3, -1);
	layout.PosAbs(&pauseButton, -2, -1);

	pausedLabel.SetCenterPos(gamui2D.Width()*0.5f, gamui2D.Height()*0.5f);

	static float SIZE_BOOST = 1.3f;
	helpImage.SetPos(helpImage.X() + layout.Width() * 0.3f, helpImage.Y() - helpImage.Height()*(SIZE_BOOST-1.0f)*0.5f);
	helpImage.SetSize(helpImage.Height()*SIZE_BOOST, helpImage.Height()*SIZE_BOOST);

	layout.PosAbs(&coreWarningIcon, 1, 6);
	layout.PosAbs(&domainWarningIcon, 1, 7);

	coreWarningIcon.SetVisible(false);
	domainWarningIcon.SetVisible(false);

	layout.PosAbs( &faceWidget, -1, 0, 1, 1 );
	layout.PosAbs( &dateLabel,   -3, 0 );
	layout.PosAbs(&summaryBars, -1, -2, 1, 1);

	layout.PosAbs( &minimap,    -2, 0, 1, 1 );
	minimap.SetSize( minimap.Width(), minimap.Width() );	// make square
	layout.PosAbs(&atlasButton, -2, 2);	// to set size and x-value
	atlasButton.SetPos(atlasButton.X(), minimap.Y() + minimap.Height());
	layout.PosAbs( &targetFaceWidget, -4, 0, 1, 1 );

	faceWidget.SetSize( faceWidget.Width(), faceWidget.Width() );
	targetFaceWidget.SetSize( faceWidget.Width(), faceWidget.Width() );

	layout.PosAbs( &moneyWidget, 5, -1 );
	techLabel.SetPos( moneyWidget.X(),	/* + moneyWidget.Width() + layout.SpacingX(),*/
					  moneyWidget.Y() + moneyWidget.Height() * 0.8f);

	static int CONSOLE_HEIGHT = 2;	// in layout...
	layout.PosAbs(&newsConsole.consoleWidget, 0, -1 - CONSOLE_HEIGHT, 1, CONSOLE_HEIGHT);

#if 0
	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		layout.PosAbs( &pickupButton[i], 0, i+3 );
	}
#endif

	layout.PosAbs(&startGameWidget, 2, 2, 5, 5);
	layout.PosAbs(&endGameWidget, 2, 2, 5, 5);

	layout.PosAbs(&abandonButton, -1, -4);
	layout.PosAbs(&abandonConfirmButton, -1, -3);

	// ------ CHANGE LAYOUT ------- //
	layout.SetSize( faceWidget.Width(), 20.0f );
	layout.SetSpacing( 5.0f );

	bool visible = game->GetDebugUI();
	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		layout.PosAbs( &newsButton[i], -1, -NUM_NEWS_BUTTONS+i );
		newsButton[i].SetVisible( visible );
	}

	allRockButton.SetVisible(visible);
	saveButton.SetVisible(visible);
	okay.SetVisible(visible);
}


void GameScene::SetBars( Chit* chit, bool isAvatar )
{
	ItemComponent* ic = chit ? chit->GetItemComponent() : 0;
	AIComponent* ai = chit ? chit->GetAIComponent() : 0;

	faceWidget.SetMeta( ic, ai );
	if (isAvatar) {
		faceWidget.SetFlags(faceWidget.Flags() & (~FaceWidget::NEED_BARS));
	}
	else {
		faceWidget.SetFlags(faceWidget.Flags() | FaceWidget::NEED_BARS);
	}
}


Engine* GameScene::GetEngine()
{
	return sim->GetEngine();
}


void GameScene::Save()
{
	const char* datPath = game->GamePath( "map", 0, "dat" );
	const char* gamePath = game->GamePath( "game", 0, "dat" );
	sim->Save( datPath, gamePath );
}


void GameScene::Load()
{
	const char* datPath  = lumosGame->GamePath( "map", 0, "dat" );
	const char* gamePath = lumosGame->GamePath( "game", 0, "dat" );

	int version = -1;
	// Get the version.
	GLString path;
	GetSystemPath(GAME_SAVE_DIR, gamePath, &path);
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp) {
		StreamReader reader(fp);
		version = reader.Version();
		fclose(fp);
		GLOUTPUT(("Current binary version=%d file version=%d\n", CURRENT_FILE_VERSION, version));
	}
	else {
		GLOUTPUT(("No game file.\n"));
	}

	if (version == CURRENT_FILE_VERSION) {
		sim->Load( datPath, gamePath );
	}
	else {
		sim->Load( datPath, 0 );
	}
	chitTracking = GetPlayerChitID();
}


void GameScene::Zoom( int style, float delta )
{
	Engine* engine = sim->GetEngine();
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void GameScene::Rotate( float degrees )
{
	sim->GetEngine()->camera.Orbit( degrees );
}


void GameScene::MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	// --- Info and debugging info. ---
	Vector3F at = { 0, 0, 0 };
	ModelVoxel mv = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, Model::MODEL_CLICK_THROUGH, 0, &at );
	MoveModel( mv.model ? mv.model->userData : 0 );

	SetSelectionModel( view );

	// Debugging output:
	ModelVoxel mv2 = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, 0, 0, &at );
	voxelInfoID = ToWorld2I(at);
	if ( mv2.model && mv2.model->userData ) {
		infoID = mv2.model->userData->ID();
	}

}


void GameScene::SetSelectionModel(const grinliz::Vector2F& view)
{
	Vector3F at = { 0, 0, 0 };
	this->ModelAtMouse(view, sim->GetEngine(), TEST_TRI, 0, 0, 0, &at);

	// --- Selection display. (Only in desktop interface.)
	float size = 1.0f;
	RenderAtom atom;

	int buildActive = menu->BuildActive();
	if (buildActive && Game::MouseMode()) {
		if (buildActive == BuildScript::CLEAR || buildActive == BuildScript::CANCEL)
		{
			//name = "clearMarker1";
			atom = LumosGame::CalcIconAtom("delete");
		}
		else {
			BuildScript buildScript;
			int s = buildScript.GetData(buildActive).size;
			if (s == 1) {
				//name = "buildMarker1";
			}
			else {
				size = 2.0f;
				//name = "buildMarker2";
			}
			atom = LumosGame::CalcIconAtom("build");
		}
	}
	selectionTile.SetPos(floorf(at.x), floorf(at.z));
	selectionTile.SetSize(size, size);
	selectionTile.SetAtom(atom);
	// Set invisible when it starts dragging.
	// Turn back on here if not dragging.
	if (dragMode == EDragMode::NONE) {
		selectionTile.SetVisible(true);
	}
}


void GameScene::ClearTargetFlags()
{
	Chit* target = sim->GetChitBag()->GetChit( targetChit );
	if ( target && target->GetRenderComponent() ) {
		target->GetRenderComponent()->SetGroundMark( 0 );
	}
	target = sim->GetChitBag()->GetChit( possibleChit );
	if ( target && target->GetRenderComponent() ) {
		target->GetRenderComponent()->SetGroundMark( 0 );
	}
	targetChit = possibleChit = 0;
}


void GameScene::MoveModel( Chit* target )
{
	Chit* player = GetPlayerChit();
	if ( !player ) {
		ClearTargetFlags();
		return;
	}

	Chit* oldTarget = sim->GetChitBag()->GetChit( possibleChit );
	if ( oldTarget ) {
		RenderComponent* rc = oldTarget->GetRenderComponent();
		if ( rc ) {
			rc->SetGroundMark( 0 );
		}
	}
	Chit* focusedTarget = sim->GetChitBag()->GetChit( targetChit );
	if ( target && target != focusedTarget ) {
		AIComponent* ai = player->GetAIComponent();

		if ( ai && Team::Instance()->GetRelationship( target, player ) == ERelate::ENEMY ) {
			possibleChit = 0;
			RenderComponent* rc = target->GetRenderComponent();
			if ( rc ) {
				rc->SetGroundMark( "possibleTarget" );
				possibleChit = target->ID();
			}
		}
	}
}


void GameScene::TapModel( Chit* target )
{
	if ( !target ) {
		return;
	}
	Chit* player = GetPlayerChit();
	if ( !player ) {
		ClearTargetFlags();
	}

	AIComponent* ai = player ? player->GetAIComponent() : 0;
	const char* setTarget = 0;

	if ( ai && Team::Instance()->GetRelationship( target, player ) == ERelate::ENEMY ) {
		ai->Target( target, true );
		setTarget = "target";
	}
	else if ( ai && Team::Instance()->GetRelationship( target, player ) == ERelate::FRIEND ) {
		const GameItem* item = target->GetItem();
		// FIXME: should use key
		bool denizen = strstr( item->ResourceName(), "human" ) != 0;
		if (denizen) {
			chitTracking = target->ID();
			//setTarget = "possibleTarget";

			CameraComponent* cc = sim->GetChitBag()->GetCamera();
			cc->SetTrack( chitTracking );
		}
	}

	if ( setTarget ) {
		ClearTargetFlags();

		RenderComponent* rc = target->GetRenderComponent();
		if ( rc ) {
			rc->SetGroundMark( setTarget );
			targetChit = target->ID();
		}
	}
}


void GameScene::ViewModel( Chit* target )
{
	if ( !target ) {
		return;
	}

	chitTracking = target->ID();
	CameraComponent* cc = sim->GetChitBag()->GetCamera();
	cc->SetTrack( chitTracking );
}


void GameScene::Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Process3DTap(action, view, world, sim->GetEngine());
	CameraComponent* cc = sim->GetChitBag()->GetCamera();
	cc->SetTrack(0);
}


void GameScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, sim->GetEngine());
	CameraComponent* cc = sim->GetChitBag()->GetCamera();
	cc->SetTrack(0);
}


void GameScene::Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	WorldMap* map = sim->GetWorldMap();

	CoreScript* coreScript = GetHomeCore();
	int uiMode = menu->UIMode();

	Vector3F plane = { 0, 0, 0 };
	int exclude = (uiMode == GameSceneMenu::UI_VIEW) ? 0 : Model::MODEL_CLICK_THROUGH;
	ModelVoxel mv = ModelAtMouse(view, sim->GetEngine(), TEST_HIT_AABB, 0, exclude, nullptr, &plane);
	Vector2I plane2i = { (int)plane.x, (int)plane.z };
	if (!map->Bounds().Contains(plane2i)) return;
	const WorldGrid& worldGrid = map->GetWorldGrid(plane2i);

	BuildAction(plane2i);

	if (coreScript && (uiMode == GameSceneMenu::UI_CONTROL)) {
		for (int i = 0; i < GameSceneMenu::NUM_SQUAD_BUTTONS; ++i) {
			if (menu->squadButton[i].Down()) {
				ControlTap(i - 1, plane2i);
			}
		}
		return;
	}

	Chit* playerChit = GetPlayerChit();
	if (uiMode == GameSceneMenu::UI_AVATAR && (worldGrid.PlantStage() >= 2 || worldGrid.RockHeight())) {
		// clicked on a rock. Melt away!
		if (playerChit && playerChit->GetAIComponent()) {
			playerChit->GetAIComponent()->Target(plane2i, false);
			return;
		}
	}

	switch (uiMode) {
		case GameSceneMenu::UI_AVATAR:
		if (mv.model) {
			TapModel(mv.model->userData);
		}
		else {
			Vector2F dest = { plane.x, plane.z };
			DoDestTapped(dest);
		}
		break;

		case GameSceneMenu::UI_VIEW:
		if (mv.model) {
			ViewModel(mv.model->userData);
		}
		break;
	}
}

bool GameScene::DragBuildArea(gamui::RenderAtom* atom)
{
	int buildActive = menu->BuildActive();
	if (buildActive && buildActive <= BuildScript::ICE && buildActive != BuildScript::ROTATE) {
		if (buildActive == BuildScript::CLEAR || buildActive == BuildScript::CANCEL)
			*atom = lumosGame->CalcIconAtom("delete");
		else
			*atom = lumosGame->CalcIconAtom("build");
		return true;
	}
	return false;
}

bool GameScene::StartDragPlanLocation(const Vector2I& at, WorkItem* workItem)
{
	CoreScript* coreScript = GetHomeCore();
	if (coreScript) {
		WorkQueue* wq = coreScript->GetWorkQueue();
		GLASSERT(wq);
		const WorkQueue::QueueItem* item = wq->HasJobAt(at);
		if (item && item->buildScriptID >= BuildScript::PAVE) {
			item->GetWorkItem(workItem);
			wq->Remove(at);
			return true;
		}
	}
	return false;
}


bool GameScene::StartDragCircuit(const grinliz::Vector2I& at)
{
	return menu->CircuitMode();
}


bool GameScene::StartDragPlanRotation(const Vector2I& at, WorkItem* workItem)
{
	CoreScript* coreScript = GetHomeCore();
	if (coreScript) {
		WorkQueue* wq = coreScript->GetWorkQueue();
		GLASSERT(wq);
		const WorkQueue::QueueItem* item = wq->HasPorchAt(at);
		if (item) {
			item->GetWorkItem(workItem);
			wq->Remove(item->pos);
			return true;
		}
	}
	return false;
}


bool GameScene::DragRotate(const grinliz::Vector2I& pos2i)
{
	Chit* building = 0;
	int uiMode = menu->UIMode();
	if ((uiMode == GameSceneMenu::UI_BUILD) && !menu->BuildActive()) {
		building = sim->GetChitBag()->QueryBuilding(IString(),pos2i,0);
		if (!building) {
			building = sim->GetChitBag()->QueryPorch(pos2i);
			if (building) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
				if (msc) {
					// Adjust the drag start to be the building, not the porch:
					mapDragStart = ToWorld2F(msc->Bounds()).Center();
				}
			}
		}
	}
	return building != 0;
}


void GameScene::DragRotateBuilding(const grinliz::Vector2F& drag)
{
	Vector2I dragStart = ToWorld2I(mapDragStart);
	Vector2I dragEnd = ToWorld2I(drag);

	Chit* building = sim->GetChitBag()->QueryBuilding(IString(), dragStart, 0);

	if (building && (dragStart != dragEnd)) {
		Vector2I d = dragEnd - dragStart;
		float rotation = 0;
		if (abs(d.x) > abs(d.y)) {
			if (d.x > 0)	rotation = 90.0f;
			else			rotation = 270.0f;
		}
		else {
			if (d.y > 0)	rotation = 0;
			else			rotation = 180.0f;
		}
		Quaternion q;
		q.FromAxisAngle(V3F_UP, rotation);
		building->SetRotation(q);
	}
}


void GameScene::DragRotatePlan(const grinliz::Vector2F& drag, WorkItem* workItem)
{
	Vector2F dragStart = ToWorld2F(workItem->pos);	// mapDragStart;
	Vector2F dragEnd = drag;

	if (dragStart != dragEnd) {
		Vector2F d = dragEnd - dragStart;
		float rotation = 0;
		if (fabs(d.x) > fabs(d.y)) {
			if (d.x > 0)	rotation = 90.0f;
			else			rotation = 270.0f;
		}
		else {
			if (d.y > 0)	rotation = 0;
			else			rotation = 180.0f;
		}
		workItem->rotation = rotation;
	}
}


void GameScene::DrawBuildMarks(const WorkItem& workItem)
{
	BuildScript buildScript;
	const BuildData& buildData = buildScript.GetData(workItem.buildScriptID);
	BuildData::DrawBounds(BuildData::Bounds(buildData.size, workItem.pos), 
						  buildData.PorchBounds(buildData.size, workItem.pos, LRint(NormalizeAngleDegrees(workItem.rotation)/90.0f)), 
						  buildMark);
}


void GameScene::DoCameraToggle()
{
	Vector3F at = V3F_ZERO;
	sim->GetEngine()->CameraLookingAt(&at);

	// Toggle high/low.
	Camera* camera = &sim->GetEngine()->camera;

	if (camera->EyeDir3()->y < -0.90f) {
		// currently high
		camera->SetQuat(savedCameraRotation);
		camera->SetPosWC(camera->PosWC().x, savedCameraHeight, camera->PosWC().z);
		sim->GetEngine()->CameraLookAt(at.x, at.z);
	}
	else {
		// currently low
		savedCameraRotation = camera->Quat();
		savedCameraHeight = camera->PosWC().y;
		camera->TiltRotationToQuat(-80, 0);
		camera->SetPosWC(camera->PosWC().x, Min(EL_CAMERA_MAX, savedCameraHeight*2.0f), camera->PosWC().z);
		sim->GetEngine()->CameraLookAt(at.x, at.z);
	}
}


void GameScene::BuildAction(const Vector2I& pos2i)
{
	CoreScript* coreScript = GetHomeCore();
	int buildActive = menu->BuildActive();
	if (coreScript && buildActive) {
		WorkQueue* wq = coreScript->GetWorkQueue();
		GLASSERT(wq);
		RemovableFilter removableFilter;

		if (buildActive == BuildScript::CLEAR) {
			wq->AddAction(pos2i, BuildScript::CLEAR, 0, 0);
			return;
		}
		else if (buildActive == BuildScript::CANCEL) {
			wq->Remove(pos2i);
		}
		else if (buildActive == BuildScript::ROTATE) {
			Chit* chit = sim->GetChitBag()->QueryBuilding(IString(),pos2i, 0);

			if (chit) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				if (msc ) {
					float r = YRotation(chit->Rotation());
					r += 90.0f;
					r = NormalizeAngleDegrees(r);
					Quaternion q = Quaternion::MakeYRotation(r);
					chit->SetRotation(q);
				}
			}
		}
		else {
			int variation = 0;
			if (buildActive == BuildScript::PAVE) {
				variation = coreScript->GetPave();
			}
			wq->AddAction(pos2i, buildActive, 0, variation);
		}
	}
}


void GameScene::ControlTap(int slot, const Vector2I& pos2i)
{
	CoreScript* cs = sim->GetChitBag()->GetHomeCore();
	if (!cs) return;

	if (slot == -1) {
		// Local control.
		Vector2I sector = ToSector(pos2i);
		Vector2I coreSector = ToSector(cs->ParentChit()->Position());
		if (sector == coreSector) {
			cs->ToggleFlag(pos2i);
		}
	}
	else {
		GLASSERT(slot >= 0 && slot < MAX_SQUADS);
		Vector2I waypoint = cs->GetWaypoint(slot);
		static const Vector2I ZERO = { 0, 0 };
		if (waypoint == pos2i) {
			cs->SetWaypoints(slot, ZERO);
		}
		else {
			cs->SetWaypoints(slot, pos2i);
		}
	}
}


bool GameScene::Tap(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	static const float CLICK_RAD = 10.0f;
	bool tapHandled = false;

	tapView = view;	// justs a temporary to pass through to ItemTapped()
	bool uiHasTap = ProcessTap(action, view, world);
//	Engine* engine = sim->GetEngine();
//	WorldMap* map = sim->GetWorldMap();
	Vector3F at = { 0, 0, 0 };
	IntersectRayAAPlane(world.origin, world.direction, 1, 0, &at, 0);

	RenderAtom atom;
	if (!uiHasTap) {
		if (action == GAME_TAP_DOWN) {
			mapDragStart = ToWorld2F(at);
			tapDown = view;

			dragMode = EDragMode::NONE;
			if (StartDragCircuit(ToWorld2I(at))) {
				dragMode = EDragMode::CIRCUIT;
				sim->Context()->physicsSims->GetCircuitSim(ToSector(at))->DragStart(ToWorld2F(at));
				tapHandled = true;
			}
			else if (DragBuildArea(&atom)) {
				dragMode = EDragMode::PLAN_AREA;
				tapHandled = true;
			}
			else if (StartDragPlanLocation(ToWorld2I(at), &dragWorkItem)) {
				dragMode = EDragMode::PLAN_MOVE;
				DrawBuildMarks(dragWorkItem);
				tapHandled = true;
			}
			else if (StartDragPlanRotation(ToWorld2I(at), &dragWorkItem)) {
				dragMode = EDragMode::PLAN_ROTATION;
				DrawBuildMarks(dragWorkItem);
				tapHandled = true;
			}
			else if (DragRotate(ToWorld2I(at))) {
				dragMode = EDragMode::BUILDING_ROTATION;
				tapHandled = true;
			}
			else {
				// This interacts badly with the other events.
				// Moved control to the main loop. (Which creates
				// the 2 finger close vs. far thing.)
				//Process3DTap(GAME_PAN_START, view, world, sim->GetEngine());

			}
		}
		else if (action == GAME_TAP_MOVE) {
			if (dragMode == EDragMode::CIRCUIT) {
				if (ToSector(at) == ToSector(mapDragStart)) {
					sim->Context()->physicsSims->GetCircuitSim(ToSector(at))->Drag(ToWorld2F(at));
				}
			}
			else if (dragMode == EDragMode::BUILDING_ROTATION) {
				DragRotateBuilding(ToWorld2F(at));
			}
			else if (dragMode == EDragMode::PLAN_AREA) {
				int count = 0;
				if ((ToWorld2F(at) - mapDragStart).LengthSquared() > 0.25f) {
					Rectangle2I r;
					r.FromPair(ToWorld2I(mapDragStart), ToWorld2I(ToWorld2F(at)));
					DragBuildArea(&atom);

					for (Rectangle2IIterator it(r); !it.Done() && count < NUM_BUILD_MARKS; it.Next(), count++) {
						buildMark[count].SetPos((float)it.Pos().x, (float)it.Pos().y);
						buildMark[count].SetSize(1.0f, 1.0f);
						buildMark[count].SetVisible(true);
						buildMark[count].SetAtom(atom);
					}
				}
				while (count < NUM_BUILD_MARKS) {
					buildMark[count].SetVisible(false);
					++count;
				}
			}
			else if (dragMode == EDragMode::PLAN_MOVE) {
				dragWorkItem.pos = ToWorld2I(at);
				DrawBuildMarks(dragWorkItem);
			}
			else if (dragMode == EDragMode::PLAN_ROTATION) {
				DragRotatePlan(ToWorld2F(at), &dragWorkItem);
				DrawBuildMarks(dragWorkItem);
			}
		}
		else if (action == GAME_TAP_UP) {
			if (dragMode == EDragMode::CIRCUIT) {
				if (ToSector(at) == ToSector(mapDragStart)) {
					if ((tapDown - view).Length() < CLICK_RAD) {	// magic tap accuracy...
						Tap3D(view, world);
					}
					else {
						CircuitSim* circuit = sim->Context()->physicsSims->GetCircuitSim(ToSector(at));
						circuit->DragEnd(ToWorld2F(at));
						circuit->Connect(ToWorld2I(mapDragStart), ToWorld2I(at));
					}
				}
				else {
					sim->Context()->physicsSims->GetCircuitSim(ToSector(at))->DragEnd(mapDragStart);
				}
			}
			else if (dragMode == EDragMode::BUILDING_ROTATION) {
				// Do nothing
			}
			else if (dragMode == EDragMode::PLAN_AREA) {
				Vector2F end = { at.x, at.z };
				Rectangle2I r;
				r.FromPair(ToWorld2I(mapDragStart), ToWorld2I(end));
				DragBuildArea(&atom);

				for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
					BuildAction(it.Pos());
				}
			}
			else if (dragMode == EDragMode::PLAN_MOVE) {
				CoreScript* cs = GetHomeCore();
				if (cs) {
					WorkQueue* workQueue = cs->GetWorkQueue();
					workQueue->AddAction(ToWorld2I(at), dragWorkItem.buildScriptID, dragWorkItem.rotation, dragWorkItem.variation);
				}
			}
			else if (dragMode == EDragMode::PLAN_ROTATION) {
				CoreScript* cs = GetHomeCore();
				if (cs) {
					WorkQueue* workQueue = cs->GetWorkQueue();
					workQueue->AddAction(dragWorkItem.pos, dragWorkItem.buildScriptID, dragWorkItem.rotation, dragWorkItem.variation);
				}
			}
			else if (dragMode == EDragMode::NONE) {
				// FIXME: is it possible to filter out at a higher level?
				// Check if the pan isn't a pan (actually a 2 finger drag) and do something.
				if ((tapDown - view).Length() < CLICK_RAD) {	// magic tap accuracy...
					Tap3D(view, world);
				}
			}
			dragMode = EDragMode::NONE;
		}
	}
	// Clear out drag indicator, if not dragging.
	if (uiHasTap || action == GAME_TAP_UP) {
		for (int i = 0; i < NUM_BUILD_MARKS; ++i) {
			buildMark[i].SetVisible(false);
		}
	}
	// Turn off if we are dragging. Turn back on in move.
	if (dragMode != EDragMode::NONE) {
		selectionTile.SetVisible(false);
	}
	return uiHasTap || (dragMode != EDragMode::NONE) || tapHandled;
}


bool GameScene::CameraTrackingAvatar()
{
	Chit* playerChit = GetPlayerChit();
	CameraComponent* cc = sim->GetChitBag()->GetCamera();
	if (playerChit && cc && (playerChit->ID() == cc->Tracking())) {
		return true;
	}
	return false;
}


bool GameScene::AvatarSelected()
{
	bool button = menu->UIMode() == GameSceneMenu::UI_AVATAR;
	Chit* playerChit = GetPlayerChit();
	if (button && playerChit) {
		return true;
	}
	return false;
}


void GameScene::DoCameraHome()
{
	CoreScript* coreScript = GetHomeCore();
	if (coreScript) {
		Chit* chit = coreScript->ParentChit();
		if (chit ) {
			Vector3F lookAt = chit->Position();
			sim->GetEngine()->CameraLookAt(lookAt.x, lookAt.z);
			CameraComponent* cc = sim->GetChitBag()->GetCamera();
			cc->SetTrack(0);
		}
	}
}


void GameScene::DoAvatarButton()
{
	CoreScript* coreScript = GetHomeCore();

	// Select.
	chitTracking = GetPlayerChitID();
	Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
	CameraComponent* cc = sim->GetChitBag()->GetCamera();
	if (cc && chit) {
		chitTracking = chit->ID();
		cc->SetTrack(chitTracking);
	}

	menu->DoEscape(true);
}


void GameScene::DoTeleportButton()
{
	CoreScript* coreScript = GetHomeCore();

	// Teleport.
	if (coreScript && GetPlayerChit()) {
		SpatialComponent::Teleport(GetPlayerChit(), coreScript->ParentChit()->Position());
	}
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	Vector2F dest = { 0, 0 };

	// Doesn't do any logic; does change the state.
	menu->ItemTapped(item);
	tutorial->ItemTapped(item);

	if (gamui2D.DialogDisplayed(startGameWidget.Name())) {
		startGameWidget.ItemTapped(item);
	}
	else if (gamui2D.DialogDisplayed(endGameWidget.Name())) {
		endGameWidget.ItemTapped(item);
	}
	else if ( item == &okay ) {
		game->PopScene();
	}
	else if (item == &pauseButton) {
		paused = !paused;
	}
	else if (item == &viewButton) {
		DoCameraToggle();
	}
	else if (item == &minimap) {
		float x = 0, y = 0;
		gamui2D.GetRelativeTap(&x, &y);
		GLOUTPUT(("minimap tapped nx=%.1f ny=%.1f\n", x, y));

		Engine* engine = sim->GetEngine();
		x *= float(engine->GetMap()->Width());
		y *= float(engine->GetMap()->Height());
		CameraComponent* cc = sim->GetChitBag()->GetCamera();
		cc->SetTrack(0);
		engine->CameraLookAt(x, y);
	}
	else if (item == &atlasButton) {
		MapSceneData* data = new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), GetPlayerChit() );
		Vector3F at;
		sim->GetEngine()->CameraLookingAt(&at);
		at.x = Clamp(at.x, 0.0f, sim->GetWorldMap()->Width() - 1.0f);
		at.z = Clamp(at.z, 0.0f, sim->GetWorldMap()->Height() - 1.0f);

		data->destSector = ToSector(ToWorld2I(at));
		CoreScript* cc = sim->GetChitBag()->GetHomeCore();
		for (int i = 0; cc && i < MAX_SQUADS; ++i) {
			data->squadDest[i] = cc->GetLastWaypoint(i);
		}
		game->PushScene( LumosGame::SCENE_MAP, data );
	}
	else if ( item == &saveButton ) {
		Save();
	}
	else if ( item == &allRockButton ) {
		sim->SetAllRock();
	}
	else if ( item == &censusButton ) {
		game->PushScene( LumosGame::SCENE_CENSUS, new CensusSceneData( sim->GetChitBag()) );
	}
	else if ( item == &menu->createWorkerButton ) {
		CoreScript* cs = GetHomeCore();
		if (cs) {
			Chit* coreChit = cs->ParentChit();
			if (coreChit->GetItem()->wallet.Gold() >= WORKER_BOT_COST) {
				ReserveBank::GetWallet()->Deposit(coreChit->GetWallet(), WORKER_BOT_COST);
				int team = coreChit->GetItem()->Team();
				sim->GetChitBag()->NewWorkerChit(coreChit->Position(), team);
			}
		}
	}
	else if (item == &abandonButton) {
		abandonConfirmButton.SetVisible(true);
	}
	else if (item == &abandonConfirmButton) {
		OpenEndGame();
		sim->GetChitBag()->SetHomeTeam(TEAM_HOUSE);
		CameraComponent* cc = sim->GetChitBag()->GetCamera();
		if (cc) cc->SetTrack(0);
		targetFaceWidget.SetFace(&uiRenderer, 0);
		endTimer = 1;	// open immediate
	}
	else if ( item == faceWidget.GetButton() ) {
		Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
		if (!chit) {
			chit = GetPlayerChit();
		}
		if ( chit && chit->GetItemComponent() ) {			
			game->PushScene( LumosGame::SCENE_CHARACTER, 
							 new CharacterSceneData( chit->GetItemComponent(), 0, 
							 chit == GetPlayerChit() ? CharacterSceneData::AVATAR : CharacterSceneData::CHARACTER_ITEM, 
							 0 ));
		}
	}
	else if ( item == &menu->useBuildingButton ) {
		sim->UseBuilding();
	}
	else if ( item == &menu->cameraHomeButton ) {
		DoCameraHome();
	}
	else if (item == &menu->uiMode[GameSceneMenu::UI_AVATAR]) {
		DoAvatarButton();
	}
	else if (item == &menu->trackAvatar) {
		DoAvatarButton();
	}
	else if (item == &menu->teleportAvatar) {
		DoTeleportButton();
	}
	else if ( item == &menu->prevUnit || item == &menu->nextUnit ) {
		CoreScript* coreScript = GetHomeCore();

		int bias = 0;
		if (item == &menu->prevUnit) bias = -1;
		if (item == &menu->nextUnit) bias = 1;

		if (coreScript ) {
			CChitArray citizens;
			int nCitizens = coreScript->Citizens(&citizens);
			if (nCitizens) {

				Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
				int index = citizens.Find(chit);

				if (index < 0)
					index = 0;
				else
					index = index + bias;

				if (index < 0) index += nCitizens;
				if (index >= nCitizens) index = 0;

				chit = citizens[index];

				CameraComponent* cc = sim->GetChitBag()->GetCamera();
				if (cc && chit) {
					chitTracking = chit->ID();
					cc->SetTrack(chitTracking);
				}
			}
		}
	}
	else if (item == &coreWarningIcon) {
		CameraComponent* cc = sim->GetChitBag()->GetCamera();
		if (cc) {
			cc->SetPanTo(coreWarningPos);
		}
	}
	else if (item == &domainWarningIcon) {
		CameraComponent* cc = sim->GetChitBag()->GetCamera();
		if (cc) {
			cc->SetPanTo(domainWarningPos);
		}
	}

	Vector2F pos2 = { 0, 0 };
	if (newsConsole.consoleWidget.IsItem(item, &pos2)) {
		if (!pos2.IsZero()) {
			CameraComponent* cc = sim->GetChitBag()->GetCamera();
			if (cc) {
				cc->SetPanTo(pos2);
			}
		}
	}

	// Only hitting the bottom row (actual action) buttons triggers
	// the build. Up until that time, the selection icon doesn't 
	// turn on.
	if (menu->UIMode() == GameSceneMenu::UI_BUILD) {
		for (int i = 1; i < BuildScript::NUM_PLAYER_OPTIONS; ++i) {
			if (&menu->buildButton[i] == item) {
				CameraComponent* cc = sim->GetChitBag()->GetCamera();
				cc->SetTrack(0);
				break;
			}
		}
	}
	SetSelectionModel(tapView);

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		if (item == &newsButton[i]) {
			auto& current = sim->GetChitBag()->GetCurrentNews();

			int index = current.Size() - 1 - i;
			if (index >= 0) {
				const ChitBag::CurrentNews& ne = current[index];
				CameraComponent* cc = sim->GetChitBag()->GetCamera();
				if (cc && ne.chitID) {
					cc->SetTrack(ne.chitID);
				}
				else {
					sim->GetEngine()->CameraLookAt(ne.pos.x, ne.pos.y);
					if (cc) cc->SetTrack(0);
				}
			}
		}
	}

#if 0
	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		if ( i < pickupData.Size() && item == &pickupButton[i] ) {
			Chit* playerChit = GetPlayerChit();
			Chit* item = sim->GetChitBag()->GetChit( pickupData[i].chitID );
			if ( item && playerChit ) {
				if ( playerChit->GetAIComponent() ) {
					playerChit->GetAIComponent()->Pickup( item );
				}
			}
		}
	}
#endif

	for (int i = 0; i < MAX_CITIZENS; ++i) {
		if (item == &menu->squadBar[i].hitBounds) {
			int id = menu->squadBar[i].userItemID;
			CameraComponent* cc = sim->GetChitBag()->GetCamera();
			if (cc && id) {
				chitTracking = id;
				cc->SetTrack(id);
			}
		}
	}

	sim->Context()->physicsSims->GetCircuitSim(GetHomeSector())->EnableOverlay(menu->CircuitMode());
	if ( !dest.IsZero() ) {
		DoDestTapped( dest );
	}
}


void GameScene::DoDestTapped( const Vector2F& _dest )
{
	Vector2F dest = _dest;

	Chit* chit = GetPlayerChit();
	if (chit) {
		AIComponent* ai = chit->GetAIComponent();
		GridMoveComponent* gridMove = GET_SUB_COMPONENT(chit, MoveComponent, GridMoveComponent);

		Vector2F pos = ToWorld2F(chit->Position());
		// Is this grid travel or normal travel?
		Vector2I currentSector = ToSector(pos.x, pos.y );
		Vector2I destSector    = ToSector( dest.x, dest.y );
		SectorPort sectorPort;

		if ( currentSector != destSector )
		{
			// Find the nearest port. (Somewhat arbitrary.)
			sectorPort.sector = destSector;
			sectorPort.port   = sim->GetWorldMap()->GetSectorData( sectorPort.sector ).NearestPort( pos );
		}

		if (gridMove) {
			if (sectorPort.IsValid())
				gridMove->SetDest(sectorPort);
		}
		else if (ai) {
			if ( sectorPort.IsValid() ) {
				ai->Move( sectorPort, true );
			}
			else if ( currentSector == destSector ) {
				ai->Move( dest, true );
			}
		}
	}
}



void GameScene::HandleHotKey( int mask )
{
	if (mask == GAME_HK_ESCAPE) {
		menu->DoEscape(false);
		SetSelectionModel(tapView);
	}
	else if (mask == GAME_HK_CAMERA_AVATAR) {
		DoAvatarButton();
	}
	else if (mask == GAME_HK_TELEPORT_AVATAR) {
		DoTeleportButton();
	}
	else if (mask == GAME_HK_CAMERA_CORE) {
		DoCameraHome();
	}
	else if (mask == GAME_HK_TOGGLE_PAUSE) {
		paused = !paused;
	}
	else if (mask == GAME_HK_DEBUG_ACTION) {
		Vector3F at = V3F_ZERO;
		sim->GetEngine()->CameraLookingAt(&at);

#ifdef DEBUG
#if 0	// Rampage player
		if ( playerChit ) {
			AIComponent* ai = playerChit->GetAIComponent();
			ai->Rampage( 0 );
		}
#endif
#if 0	// Create Gob domain.
		{
			CoreScript* cs = CoreScript::GetCore(ToSector(ToWorld2F(at)));
			if (cs) {
				Vector2I sector = cs->ParentChit()->GetSector();
				int team = Team::GenTeam(TEAM_GOB);
				cs = sim->CreateCore(sector, team);
				cs->ParentChit()->Add(new GobDomainAI());
				Transfer(&cs->ParentChit()->GetItem()->wallet, &ReserveBank::Instance()->bank, 150);
			}
		}
#endif
#if 0	// Create Kamakira domain.
		{
			CoreScript* cs = CoreScript::GetCore(ToSector(ToWorld2F(at)));
			if (cs) {
				Vector2I sector = cs->ParentChit()->GetSector();
				int team = Team::GenTeam(TEAM_KAMAKIRI);
				cs = CoreScript::CreateCore(sector, team, sim->Context());
				cs->ParentChit()->Add(new KamakiriDomainAI());
				cs->ParentChit()->GetWallet()->Deposit(ReserveBank::GetWallet(), 1000);

				for (int i = 0; i<5; ++i) {
					sim->GetChitBag()->NewDenizen(ToWorld2I(at), TEAM_KAMAKIRI);
					at.x += 0.5f;
				}
			}
		}
#endif
#if 0	// Create Human domain.
		{
			CoreScript* cs = CoreScript::GetCore(ToSector(ToWorld2F(at)));
			if (cs) {
				Vector2I sector = ToSector(cs->ParentChit()->Position());
				int team = Team::GenTeam(TEAM_HOUSE);
				cs = CoreScript::CreateCore(sector, team, sim->Context());
				cs->ParentChit()->Add(new HumanDomainAI());
				cs->ParentChit()->GetWallet()->Deposit(ReserveBank::GetWallet(), 1000);

				for (int i = 0; i<5; ++i) {
					sim->GetChitBag()->NewDenizen(ToWorld2I(at), TEAM_HOUSE);
					at.x += 0.5f;
				}
			}
		}
#endif
#if 0	// Unsettle fluids
		{	
			Vector2I sector = ToSector(ToWorld2I(at));
			sim->GetWorldMap()->Unsettle(sector);
		}
#endif
#if 0	// Look at Truulga
		{
			Chit* truulga = sim->GetChitBag()->GetDeity(LumosChitBag::DEITY_TRUULGA);
			if (truulga) {
				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
				cc->SetTrack(0);
				Vector2F lookat = truulga->etPosition2D();
				sim->GetEngine()->CameraLookAt(lookat.x, lookat.y);
			}
		}
#endif
#if 1	// Monster swarm
		for (int i = 0; i<5; ++i) {
			sim->GetChitBag()->NewMonsterChit(at, "mantis", TEAM_GREEN_MANTIS);
			at.x += 0.5f;
		}
#endif
#if 0	// Look at pools.
		Vector2I loc = sim->GetWorldMap()->GetPoolLocation(poolView);
		if (loc.IsZero()) {
			poolView = 0;
		}
		++poolView;
		Vector2F lookAt = ToWorld2F(loc);
		sim->GetEngine()->CameraLookAt(lookAt.x, lookAt.y);
#endif
#if 0	// Summon Greaters
		if (playerChit) {
			sim->GetChitBag()->AddSummoning(playerChit->GetSector(), LumosChitBag::SUMMON_TECH);
		}
#endif
#endif	// DEBUG
	}
	else if (mask == GAME_HK_CAMERA_TOGGLE) {
		DoCameraToggle();
	}
	else if (mask == GAME_HK_ATTACH_CORE) {
		Vector3F at = V3F_ZERO;
		sim->GetEngine()->CameraLookingAt(&at);
		attached = ToSector(ToWorld2I(at));
	}
	else if ( mask == GAME_HK_TOGGLE_PATHING ) {
		Rectangle2I r;
		if ( sim->GetWorldMap()->IsShowingRegionOverlay() ) {
			r.Set( 0, 0, 0, 0 );
		}
		else {
			Vector3F at;
			sim->GetEngine()->CameraLookingAt( &at );
			Vector2I sector = ToSector(at.x, at.z);
			r.Set( sector.x*SECTOR_SIZE, sector.y*SECTOR_SIZE, (sector.x+1)*SECTOR_SIZE-1, (sector.y+1)*SECTOR_SIZE-1 );
		}
		sim->GetWorldMap()->ShowRegionOverlay( r );
	}
	else if ( mask == GAME_HK_MAP ) {
		Chit* playerChit = GetPlayerChit();
		MapSceneData* data = new MapSceneData(sim->GetChitBag(), sim->GetWorldMap(), playerChit);
		CoreScript* cc = sim->GetChitBag()->GetHomeCore();
		for (int i = 0; cc && i < MAX_SQUADS; ++i) {
			data->squadDest[i] = cc->GetLastWaypoint(i);
		}
		game->PushScene( LumosGame::SCENE_MAP, data);
	}
	else if ( mask == GAME_HK_CHEAT_GOLD ) {
		CoreScript* cs = GetHomeCore();
		if ( cs ) {
			static const int GOLD = 100;
			cs->ParentChit()->GetWallet()->Deposit(ReserveBank::GetWallet(), GOLD);
		}
	}
	else if (mask == GAME_HK_CHEAT_ELIXIR) {
//		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
//		if (cs) {
//			cs->nElixir += 20;
//		}
	}
	else if ( mask == GAME_HK_CHEAT_CRYSTAL ) {
		CoreScript* cs = GetHomeCore();
		if ( cs ) {
			int crystal[NUM_CRYSTAL_TYPES] = { 1 };
			cs->ParentChit()->GetWallet()->Deposit(ReserveBank::GetWallet(), 0, crystal);
		}
	}
	else if ( mask == GAME_HK_CHEAT_TECH ) {
		CoreScript* coreScript = GetHomeCore();
		if (coreScript) {
			for (int i = 0; i < 10; ++i) {
				coreScript->AddTech();
			}
		}
	}
	else if ( mask == GAME_HK_CHEAT_HERD ) {
		Vector3F at;
		sim->GetEngine()->CameraLookingAt( &at );
		Vector2I sector = ToSector( ToWorld2I( at ));
		ForceHerd(sector, 0);
	}
	else {
		super::HandleHotKey( mask );
	}
}


void GameScene::ForceHerd(const grinliz::Vector2I& sector, int team)
{
	CDynArray<Chit*> arr;
	MOBKeyFilter filter;
	Rectangle2F bounds = ToWorld2F(InnerSectorBounds(sector));
	sim->GetChitBag()->QuerySpatialHash(&arr, bounds, 0, &filter);

	for (int i = 0; i<arr.Size(); ++i) {
		IString mob = arr[i]->GetItem()->keyValues.GetIString(ISC::mob);
		if (!mob.empty() && Team::Instance()->GetRelationship(team, arr[i]->Team()) == ERelate::ENEMY) {
			AIComponent* ai = arr[i]->GetAIComponent();
			ai->GoSectorHerd(true);
		}
	}
}


#if 0
void GameScene::SetBuildButtons(const int* arr)
{
	// Went back and forth a bit on whether this should be
	// BuildScript. But in the end, the enable/disbale is 
	// actually for helping the player and clarifying the
	// UI, not core game logic.

	int nBeds = arr[BuildScript::SLEEPTUBE];
	int nTemples = arr[BuildScript::TEMPLE];
	int nFarms = arr[BuildScript::FARM];
	int nDistilleries = arr[BuildScript::DISTILLERY];
	int nMarkets = arr[BuildScript::MARKET];
	int nCircuitFab = arr[BuildScript::CIRCUITFAB];

	// Enforce the sleep tube limit.
	CStr<32> str;
	int techLevel = Min(nTemples, 3);
	int maxTubes = CoreScript::MaxCitizens(nTemples);

	BuildScript buildScript;
	const BuildData* sleepTubeData = buildScript.GetDataFromStructure(ISC::bed, 0);
	GLASSERT(sleepTubeData);

	str.Format( "SleepTube\n%d %d/%d", sleepTubeData->cost, nBeds, maxTubes );
	buildButton[BuildScript::SLEEPTUBE].SetText( str.c_str() ); 
	buildButton[BuildScript::SLEEPTUBE].SetEnabled(nBeds < maxTubes);

	buildButton[BuildScript::TEMPLE].SetEnabled(nBeds > 0);
	buildButton[BuildScript::GUARDPOST].SetEnabled(nTemples > 0);
	buildButton[BuildScript::BAR].SetEnabled(nDistilleries > 0);
	buildButton[BuildScript::DISTILLERY].SetEnabled(nFarms > 0);
	buildButton[BuildScript::FORGE].SetEnabled(nMarkets > 0);
	buildButton[BuildScript::EXCHANGE].SetEnabled(nMarkets > 0);
	buildButton[BuildScript::VAULT].SetEnabled(nMarkets > 0);
	buildButton[BuildScript::KIOSK].SetEnabled(nTemples > 0);

	buildButton[BuildScript::BATTERY].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::TURRET].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_SWITCH].SetEnabled(nCircuitFab > 0);
//	buildButton[BuildScript::BUILD_CIRCUIT_ZAPPER].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_BEND].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUTI_FORK_2].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_ICE].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_STOP].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_DETECT_ENEMY].SetEnabled(nCircuitFab > 0);
	buildButton[BuildScript::BUILD_CIRCUIT_TRANSISTOR].SetEnabled(nCircuitFab > 0);
}
#endif


void GameScene::DoTick(U32 delta)
{
	int buildingCounts[BuildScript::NUM_PLAYER_OPTIONS] = { 0 };
	sim->GetChitBag()->BuildingCounts(GetHomeSector(), buildingCounts, BuildScript::NUM_PLAYER_OPTIONS);

	if (!paused) {
		sim->DoTick(delta);
	}
	menu->DoTick(GetHomeCore(), buildingCounts, BuildScript::NUM_PLAYER_OPTIONS);
	//	SetPickupButtons();

	for (int i = 0; i < NUM_NEWS_BUTTONS; ++i) {
		const auto& current = sim->GetChitBag()->GetCurrentNews();

		int index = current.Size() - 1 - i;
		if (index >= 0) {
			const ChitBag::CurrentNews& ne = current[index];
			IString name = ne.name;
			newsButton[i].SetText(name.c_str());
			newsButton[i].SetEnabled(true);
		}
		else {
			newsButton[i].SetText("");
			newsButton[i].SetEnabled(false);
		}
	}

	Vector3F lookAt = { 0, 0, 0 };
	sim->GetEngine()->CameraLookingAt(&lookAt);
	Vector2I viewingSector = ToSector(lookAt.x, lookAt.z);
	const SectorData& sd = sim->GetWorldMap()->GetWorldInfo().GetSectorData(viewingSector);

	CoreScript* homeCoreScript = GetHomeCore();

	// Set the states: VIEW, BUILD, AVATAR. Avatar is 
	// disabled if there isn't one...
	Chit* playerChit = GetPlayerChit();
	menu->EnableBuildAndControl(homeCoreScript != 0);

	Chit* track = sim->GetChitBag()->GetChit(chitTracking);
	if (!track && GetPlayerChit()) {
		track = GetPlayerChit();
	}
	faceWidget.SetFace(&uiRenderer, track ? track->GetItem() : 0);

	if (playerChit && playerChit->GetAIComponent() && playerChit->GetAIComponent()->GetTarget()) {
		Chit* target = playerChit->GetAIComponent()->GetTarget();
		GLASSERT(target->GetItem());
		targetFaceWidget.SetFace(&uiRenderer, target->GetItem());
		targetFaceWidget.SetMeta(target->GetItemComponent(), target->GetAIComponent());
	}
	else {
		targetFaceWidget.SetFace(&uiRenderer, 0);
	}

	SetBars(track, track == playerChit);
	CStr<64> str;

	if (homeCoreScript) {
		double sum[ai::Needs::NUM_NEEDS + 1] = { 0 };
		bool critical[ai::Needs::NUM_NEEDS + 1] = { 0 };
		int nActive = 0;
		Vector2I sector = ToSector(homeCoreScript->ParentChit()->Position());

		CChitArray citizens;
		homeCoreScript->Citizens(&citizens);

		if (citizens.Size()) {
			for (int i = 0; i < citizens.Size(); i++) {
				Chit* chit = citizens[i];
				if (chit && chit != playerChit && ToSector(chit->Position()) == sector && chit->GetAIComponent()) {
					++nActive;
					const ai::Needs& needs = chit->GetAIComponent()->GetNeeds();
					for (int k = 0; k < ai::Needs::NUM_NEEDS; ++k) {
						sum[k] += needs.Value(k);
						if (needs.Value(k) < 0.05) {
							critical[k] = true;
						}
					}
					sum[ai::Needs::NUM_NEEDS] += needs.Morale();
					if (needs.Morale() < 0.1) {
						critical[ai::Needs::NUM_NEEDS] = true;
					}
				}
			}
			if (nActive) {
				for (int k = 0; k < ai::Needs::NUM_NEEDS; ++k) {
					sum[k] /= double(nActive);
				}
				sum[ai::Needs::NUM_NEEDS] /= double(nActive);
			}
		}
		RenderAtom blue = LumosGame::CalcPaletteAtom(8, 0);
		RenderAtom red = LumosGame::CalcPaletteAtom(0, 1);
		summaryBars.barArr[0]->SetAtom(0, critical[ai::Needs::NUM_NEEDS] ? red : blue);
		summaryBars.barArr[0]->SetRange(float(sum[ai::Needs::NUM_NEEDS]));
		for (int k = 0; k < ai::Needs::NUM_NEEDS; ++k) {
			summaryBars.barArr[k + 1]->SetAtom(0, critical[k] ? red : blue);
			summaryBars.barArr[k + 1]->SetRange(float(sum[k]));
		}

		str.Format("Date %.2f\n%s\nPop %d/%d",
				   sim->AgeF(),
				   sd.name.safe_str(),
				   citizens.Size(), CoreScript::MaxCitizens(buildingCounts[BuildScript::TEMPLE]));
		dateLabel.SetText(str.c_str());
	}
	else {
		str.Format("Date %.2f\n%s",
				   sim->AgeF(),
				   sd.name.safe_str());
		dateLabel.SetText(str.c_str());
	}

	pausedLabel.SetVisible(paused);
	str.Clear();

	Wallet wallet;
	CoreScript* cs = GetHomeCore();
	if (cs) {
		wallet = cs->ParentChit()->GetItem()->wallet;
	}
	moneyWidget.Set(wallet);

	str.Clear();
	if (playerChit && playerChit->GetItem()) {
		const GameTrait& stat = playerChit->GetItem()->Traits();
		str.Format("Level %d XP %d/%d", stat.Level(), stat.Experience(), GameTrait::LevelToExperience(stat.Level() + 1));
	}

	bool abandonVisible = (menu->UIMode() == GameSceneMenu::UI_BUILD) && homeCoreScript;
	abandonButton.SetVisible(abandonVisible);
	if (!abandonVisible) {
		abandonConfirmButton.SetVisible(false);
	}

	str.Clear();

	if (homeCoreScript) {
		float tech = homeCoreScript->GetTech();
		int maxTech = homeCoreScript->MaxTech();
		str.Format("Tech %.2f / %d", tech, maxTech);
		techLabel.SetText(str.c_str());
	}
	else {
		techLabel.SetText("Tech 0 / 0");
	}

	CheckGameStage(delta);
	int nWorkers = 0;

	Vector2I homeSector = GetHomeSector();
	{
		// Enforce the worker limit.
		CStr<32> str2;

		Rectangle2F b = ToWorld2F(InnerSectorBounds(homeSector));
		CChitArray arr;
		ItemNameFilter workerFilter(ISC::worker);
		sim->GetChitBag()->QuerySpatialHash(&arr, b, 0, &workerFilter);
		nWorkers = arr.Size();
		menu->SetNumWorkers(nWorkers);
	}

	{
		LumosChitBag* cb = sim->GetChitBag();
		Vector2I sector = GetHomeSector();
		int arr[BuildScript::NUM_PLAYER_OPTIONS] = { 0 };
		cb->BuildingCounts(sector, arr, BuildScript::NUM_PLAYER_OPTIONS);
		adviser->DoTick(delta, homeCoreScript, nWorkers, arr, BuildScript::NUM_PLAYER_OPTIONS);
	}

	newsConsole.DoTick(delta, sim->GetChitBag()->GetHomeCore());

	bool useBuildingVisible = false;
	if (playerChit) {
		Chit* building = sim->GetChitBag()->QueryPorch(ToWorld2I(playerChit->Position()));
		if (building) {
			IString name = building->GetItem()->IName();
			if (name == ISC::vault || name == ISC::factory || name == ISC::market || name == ISC::exchange
				|| name == ISC::switchOff || name == ISC::switchOn)
			{
				useBuildingVisible = true;
			}
		}
	}
	menu->SetUseBuilding(useBuildingVisible);
	menu->SetCanTeleport(playerChit != 0);
	sim->GetEngine()->RestrictCamera(0);

	// The game will open scenes - say the CharacterScene for the
	// Vault - in response to player actions. Look for them here.
	if (!game->IsScenePushed()) {
		int id = 0;
		SceneData* data = 0;
		if (sim->GetChitBag()->PopScene(&id, &data)) {
			game->PushScene(id, data);
		}
	}

	coreWarningIcon.SetVisible(coreWarningTimer > 0);
	coreWarningTimer -= delta;

	domainWarningIcon.SetVisible(domainWarningTimer > 0);
	domainWarningTimer -= delta;
}




void GameScene::OpenEndGame()
{
	CoreScript* cs = sim->GetChitBag()->GetHomeCore();
	GLASSERT(cs);
	if (!cs) return;

	Vector2I sector = ToSector(cs->ParentChit()->Position());
	const SectorData& sd = sim->GetWorldMap()->GetSectorData(sector);
	endGameWidget.SetData(sim->GetChitBag()->GetNewsHistory(),
						  this,
						  sd.name, cs->ParentChit()->GetItem()->ID(),
						  cs->GetAchievement());
	endTimer = 8 * 1000;
}

void GameScene::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_DESTROYED) {
		if (chit->GetComponent("CoreScript")) {
			if (sim->GetChitBag()->GetHomeTeam() && (chit->Team() == sim->GetChitBag()->GetHomeTeam())) {
				SetBars(0, false);
				CameraComponent* cc = sim->GetChitBag()->GetCamera();
				if (cc) cc->SetTrack(0);
				OpenEndGame();
			}
		}
	}
	else if (msg.ID() == ChitMsg::CHIT_DAMAGE 
		&& sim->GetChitBag()->GetHomeTeam()
		&& chit->Team() == sim->GetChitBag()->GetHomeTeam() ) 
	{
		BuildingFilter filter;
		if (filter.Accept(chit)) {
			if (chit->GetComponent("CoreScript")) {
				coreWarningTimer = 6000;
				coreWarningPos = ToWorld2F(chit->Position());
			}
			else {
				domainWarningTimer = 6000;
				domainWarningPos = ToWorld2F(chit->Position());
			}
		}
	}
}

/*
	Startup: !domain && !endWidget.Visible
			 if (!startWidget.Visible )
				create startup data
				make data
			creates domain.
			goes !Visible
	Play:	has domain.
			at end creates EndGameData
			go endWidget.Visible
	End:	endWidget.Visible
			goes !Visible when done
*/
void GameScene::CheckGameStage(U32 delta)
{
	CoreScript* cs = sim->GetChitBag()->GetHomeCore();
	bool endVisible   = endTimer || gamui2D.DialogDisplayed(endGameWidget.Name());
	bool startVisible = gamui2D.DialogDisplayed(startGameWidget.Name());

	if (endTimer) {
		endTimer -= delta;
		if (endTimer <= 0) {
			gamui2D.PushDialog(endGameWidget.Name());
			endTimer = 0;
		}
	}
	else if (!cs && !startVisible && !endVisible && !tutorial->Visible()) {
		// Try to find a suitable starting location.
		Rectangle2I b;
		Random random;
		random.SetSeedFromTime();

		b.min.y = b.min.x = (NUM_SECTORS / 2) - NUM_SECTORS / 4;
		b.max.y = b.max.x = (NUM_SECTORS / 2) + NUM_SECTORS / 4;

		CArray<SectorInfo, NUM_SECTORS * NUM_SECTORS> arr;
		for (Rectangle2IIterator it(b); !it.Done() && arr.HasCap(); it.Next()) {
			const SectorData* sd = &sim->GetWorldMap()->GetSectorData(it.Pos());
			if (sd->HasCore() && arr.HasCap()) {
				Vector2I sector = sd->sector;
				CoreScript* cs = CoreScript::GetCore(sector);
				if (cs && cs->ParentChit()->Team() == TEAM_NEUTRAL) {
					Rectangle2I bi = sd->InnerBounds();
					int bioFlora = 0;
					for (Rectangle2IIterator pit(bi); !pit.Done(); pit.Next()) {
						const WorldGrid& wg = sim->GetWorldMap()->GetWorldGrid(pit.Pos().x, pit.Pos().y);
						if (wg.Plant() && wg.PlantStage() >= 2) {	// only count the grown ones??
							++bioFlora;
						}
					}
					SectorInfo* si = arr.PushArr(1);
					si->sectorData = sd;
					si->bioFlora = bioFlora;
				}
			}
		}
		if (arr.Size()) {
			arr.Sort();
			// Randomize the first 16 - it isn't all about bioflora
			random.ShuffleArray(arr.Mem(), Min(arr.Size(), 16));

			// FIXME all needlessly complicated
			CArray<const SectorData*, 16> sdarr;
			for (int i = 0; i < arr.Size() && sdarr.HasCap(); ++i) {
				sdarr.Push(arr[i].sectorData);
			}

			startGameWidget.SetSectorData(sdarr.Mem(), sdarr.Size(), sim->GetEngine(), sim->GetChitBag(), this, sim->GetWorldMap());
			gamui2D.PushDialog(startGameWidget.Name());

			CameraComponent* cc = sim->GetChitBag()->GetCamera();
			cc->SetTrack(0);
		}
	}
}


void GameScene::DialogResult(const char* name, void* data)
{
	if (StrEqual(name, startGameWidget.Name())) {
		GLOUTPUT(("Selected scene.\n"));

		const SectorData* sd = (const SectorData*)data;
		//CoreScript* cs = CoreScript::GetCore(ToSector(sd->x, sd->y));
		//cs->ParentChit()->GetItem()->primaryTeam = TEAM_HOUSE0;
		int team = Team::Instance()->GenTeam(TEAM_HOUSE);
		sim->GetChitBag()->SetHomeTeam(team);
		CoreScript::CreateCore(sd->sector, team, sim->Context());
		ForceHerd(sd->sector, team);

		ReserveBank* bank = ReserveBank::Instance();
		if (bank) {
			CoreScript* cs = CoreScript::GetCore(sd->sector);
			GLASSERT(cs);
			Chit* parent = cs->ParentChit();
			GLASSERT(parent);
			GameItem* item = parent->GetItem();
			GLASSERT(item);
			item->wallet.Deposit(&bank->wallet, 250);
		}
	}
	gamui2D.PopDialog();
}


void GameScene::Draw3D( U32 deltaTime )
{
	sim->Draw3D(paused ? 0 : deltaTime);

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					 (const void*)sim->GetMiniMapTexture(), 
					 0, 1, 1, 0 );		// flip upside down!

	/* coordinate test. they are correct.
	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					 (const void*)TextureManager::Instance()->GetTexture( "palette" ),
					 0, 0, 1, 1 );
	*/

	minimap.SetAtom( atom );
	Vector3F lookAt = { 0, 0, 0 };
	sim->GetEngine()->CameraLookingAt( &lookAt );
	Map* map = sim->GetEngine()->GetMap();
	
	float x = minimap.X() + Lerp( 0.f, minimap.Width(),  lookAt.x / (float)map->Width() );
	float y = minimap.Y() + Lerp( 0.f, minimap.Height(), lookAt.z / (float)map->Height() );

	playerMark.SetCenterPos( x, y );
}


void GameScene::DrawDebugText()
{
	static const int x = 0;
	int y = 200;
	DrawDebugTextDrawCalls(x, y, sim->GetEngine());
	y += 16;

	UFOText* ufoText = UFOText::Instance();
	Chit* chit = GetPlayerChit();
	Engine* engine = sim->GetEngine();
	LumosChitBag* chitBag = sim->GetChitBag();
	WorldMap* worldMap = sim->GetWorldMap();

	if (GetPlayerChit()) {
		const Vector3F& v = chit->Position();
		ufoText->Draw(x, y, "Player: %.1f, %.1f, %.1f  Camera: %.1f %.1f %.1f",
					  v.x, v.y, v.z,
					  engine->camera.PosWC().x, engine->camera.PosWC().y, engine->camera.PosWC().z);
		y += 16;
	}
	Vector3F at = { 0, 0, 0 };
	engine->CameraLookingAt(&at);

	const Wallet& w = ReserveBank::Instance()->wallet;
	double fractionTicked = 0;
	if (chitBag->NumChits()) fractionTicked = double(chitBag->NumTicked()) / double(chitBag->NumChits());
	ufoText->Draw(x, y, "ticks=%02d%% (%d) Reserve Au=%d G=%d R=%d B=%d V=%d",
				  LRint(fractionTicked*100.0), chitBag->NumChits(),
				  w.Gold(), w.Crystal(0), w.Crystal(1), w.Crystal(2), w.Crystal(3));
	y += 16;

	int typeCount[NUM_PLANT_TYPES];
	for (int i = 0; i < NUM_PLANT_TYPES; ++i) {
		typeCount[i] = 0;
		for (int j = 0; j < MAX_PLANT_STAGES; ++j) {
			typeCount[i] += worldMap->plantCount[i][j];
		}
	}
	int stageCount[MAX_PLANT_STAGES];
	for (int i = 0; i < MAX_PLANT_STAGES; ++i) {
		stageCount[i] = 0;
		for (int j = 0; j < NUM_PLANT_TYPES; ++j) {
			stageCount[i] += worldMap->plantCount[j][i];
		}
	}

	int lesser = 0, greater = 0, denizen = 0;
	chitBag->census.NumByType(&lesser, &greater, &denizen);

	//ufoText->Draw( x, y,	"Plants type: %d %d %d %d %d %d %d %d stage: %d %d %d %d AIs: lesser=%d greater=%d denizen=%d", 
	//								typeCount[0], typeCount[1], typeCount[2], typeCount[3], typeCount[4], typeCount[5], typeCount[6], typeCount[7],
	//								stageCount[0], stageCount[1], stageCount[2], stageCount[3],
	//								lesser, greater, denizen );
	//y += 16;

	CoreScript* cs = CoreScript::GetCore(ToSector(at));
	int teamID = 0, teamGroup = 0;
	if (cs) {
		Team::SplitID(cs->ParentChit()->Team(), &teamGroup, &teamID);
	}
	ufoText->Draw(x, y, "CoreScript: %p inUse=%d team=%d,%d", cs, cs && cs->InUse() ? 1 : 0, teamGroup, teamID);
	y += 16;

	micropather::CacheData cacheData;
	Vector2I sector = ToSector(at.x, at.z);
	sim->GetWorldMap()->PatherCacheHitMiss(sector, &cacheData);
	ufoText->Draw(x, y, "Pather(%d,%d) kb=%d/%d %.2f cache h:m=%d:%d %.2f",
				  sector.x, sector.y,
				  cacheData.nBytesUsed / 1024,
				  cacheData.nBytesAllocated / 1024,
				  cacheData.memoryFraction,
				  cacheData.hit,
				  cacheData.miss,
				  cacheData.hitFraction);
	y += 16;

	Chit* info = sim->GetChitBag()->GetChit(infoID);
	if (info) {
		GLString str;
		info->DebugStr(&str);
		ufoText->Draw(x, y, "id=%d: %s", infoID, str.c_str());
		y += 16;
	}
	if (!voxelInfoID.IsZero()) {
		Vector2I sector = ToSector(voxelInfoID);
		const WorldGrid& wg = sim->GetWorldMap()->GetWorldGrid(voxelInfoID.x, voxelInfoID.y);
		ufoText->Draw(x, y, "voxel=%d,%d (%d,%d %d) hp=%d/%d pave=%d ",
					  voxelInfoID.x, voxelInfoID.y, 
					  sector.x, sector.y,
					  sector.y * NUM_SECTORS + sector.x,
					  wg.HP(), wg.TotalHP(), wg.Pave());
		y += 16;
	}
}


void GameScene::SceneResult( int sceneID, int result, const SceneData* data )
{
	if ( sceneID == LumosGame::SCENE_CHARACTER ) {
		// Hardpoint setting is now automatic
	}
	else if ( sceneID == LumosGame::SCENE_MAP ) {
		const MapSceneData* msd = (const MapSceneData*)data;
		// Works like a tap. Reconstruct map coordinates.
		Vector2I sector = msd->destSector;
		if ( !sector.IsZero() ) {
			Vector2F dest = { float(sector.x*SECTOR_SIZE + SECTOR_SIZE/2), float(sector.y*SECTOR_SIZE + SECTOR_SIZE/2) };
			if (msd->view) {
				sim->GetEngine()->CameraLookAt(dest.x, dest.y);
				CameraComponent* cc = sim->GetChitBag()->GetCamera();
				cc->SetTrack(0);
			}
			else {
				DoDestTapped(dest);
			}
		}
	}
}


Chit* GameScene::GetPlayerChit()
{
	if (attached.IsZero()) {
		return sim->GetChitBag()->GetAvatar();
	}
	return 0;	// don't have avatars when attached.
}


int GameScene::GetPlayerChitID()
{
	Chit* chit = GetPlayerChit();
	return chit ? chit->ID() : 0;
}


CoreScript* GameScene::GetHomeCore()
{
	if (attached.IsZero()) {
		return sim->GetChitBag()->GetHomeCore();
	}
	else {
		CoreScript* cs = CoreScript::GetCore(attached);
		return cs;
	}
}


grinliz::Vector2I GameScene::GetHomeSector()
{
	if (attached.IsZero()) {
		return sim->GetChitBag()->GetHomeSector();
	}
	else {
		CoreScript* cs = CoreScript::GetCore(attached);
		if (cs) {
			return ToSector(cs->ParentChit()->Position());
		}
	}
	Vector2I z = { 0, 0 };
	return z;
}

