
#include <time.h>

#include "gamescene.h"
#include "characterscene.h"

#include "../version.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/cameracomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

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

#include "../engine/engine.h"
#include "../engine/text.h"

#include "../script/procedural.h"
#include "../script/corescript.h"
#include "../script/itemscript.h"
#include "../script/plantscript.h"

#include "../scenes/mapscene.h"
#include "../scenes/censusscene.h"

#include "../ai/rebuildai.h"

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 1.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	fastMode = false;
	simTimer = 0;
	simPS = 1.0f;
	simCount = 0;
	targetChit = 0;
	possibleChit = 0;
	infoID = 0;
	selectionModel = 0;
	buildActive = 0;
	chitTracking = 0;
	currentNews = 0;
	endTimer = 0;
	coreWarningTimer = 0;
	domainWarningTimer = 0;
	poolView = 0;
	voxelInfoID.Zero();
	lumosGame = game;
	game->InitStd( &gamui2D, &okay, 0 );
	sim = new Sim( lumosGame );

	Load();
	
	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );
	minimap.SetCapturesTap( true );

	atlasButton.Init(&gamui2D, game->GetButtonLook(0));
	atlasButton.SetText("Atlas");

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );

	saveButton.Init( &gamui2D, game->GetButtonLook(0) );
	saveButton.SetText( "Save" );

	loadButton.Init(&gamui2D, game->GetButtonLook(0));
	loadButton.SetText("Load");

	useBuildingButton.Init(&gamui2D, game->GetButtonLook(0));
	useBuildingButton.SetText( "Use" );
	useBuildingButton.SetVisible( false );

	cameraHomeButton.Init( &gamui2D, game->GetButtonLook(0) );
	cameraHomeButton.SetText( "Home" );
	cameraHomeButton.SetVisible( false );

	nextUnit.Init( &gamui2D, game->GetButtonLook( 0 ));
	nextUnit.SetText( ">" );
	nextUnit.SetVisible( false );

	prevUnit.Init( &gamui2D, game->GetButtonLook( 0 ));
	prevUnit.SetText( "<" );
	prevUnit.SetVisible( false );

	avatarUnit.Init(&gamui2D, game->GetButtonLook(0));
	avatarUnit.SetText("Avatar");
	avatarUnit.SetVisible(false);

	helpText.Init(&gamui2D);

	static const char* modeButtonText[NUM_BUILD_MODES] = {
		"Utility", "Denizen", "Agronomy", "Economy", "Visitor", "Circuits"
	};
	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		modeButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		modeButton[i].SetText( modeButtonText[i] );
		modeButton[0].AddToToggleGroup( &modeButton[i] );
	}
	for( int i=0; i<BuildScript::NUM_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );

		buildButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		buildButton[i].SetText( bd.label.safe_str() );

		if (bd.zoneCreate == BuildData::ZONE_INDUSTRIAL || bd.zoneConsume == BuildData::ZONE_INDUSTRIAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("anvil", true), game->CalcUIIconAtom("anvil", false));
		else if (bd.zoneCreate == BuildData::ZONE_NATURAL || bd.zoneConsume == BuildData::ZONE_NATURAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("leaf", true), game->CalcUIIconAtom("leaf", false));

		buildButton[0].AddToToggleGroup( &buildButton[i] );
		modeButton[bd.group].AddSubItem( &buildButton[i] );
	}
	buildButton[0].SetText("UNUSED");

	tabBar0.Init( &gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );
	tabBar1.Init( &gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );

	createWorkerButton.Init( &gamui2D, game->GetButtonLook(0) );
	createWorkerButton.SetText( "WorkerBot" );

	autoRebuild.Init(&gamui2D, game->GetButtonLook(0));
	autoRebuild.SetText("Auto\nRebuild");
	autoRebuild.SetVisible(false);

	abandonButton.Init(&gamui2D, game->GetButtonLook(0));
	abandonButton.SetText("Abandon");
	abandonButton.SetVisible(false);

	buildDescription.Init(&gamui2D);

	for( int i=0; i<NUM_UI_MODES; ++i ) {
		static const char* TEXT[NUM_UI_MODES] = { "View", "Build", "Control" };
		uiMode[i].Init( &gamui2D, game->GetButtonLook(0));
		uiMode[i].SetText( TEXT[i] );
		uiMode[0].AddToToggleGroup( &uiMode[i] );
	}
	
	allRockButton.Init( &gamui2D, game->GetButtonLook(0) );
	allRockButton.SetText( "All Rock" );
	censusButton.Init( &gamui2D, game->GetButtonLook(0) );
	censusButton.SetText( "Census" );

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		newsButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		newsButton[i].SetSize( NEWS_BUTTON_WIDTH, NEWS_BUTTON_HEIGHT );
		newsButton[i].SetText( "news" );
	}
	swapWeapons.Init(&gamui2D, game->GetButtonLook(0));
	swapWeapons.SetText("Swap\nWeapons");

	faceWidget.Init( &gamui2D, game->GetButtonLook(0), FaceWidget::ALL );
	faceWidget.SetSize( 100, 100 );

	chitTracking = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;

	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		pickupButton[i].Init( &gamui2D, game->GetButtonLook(0) );
	}

	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	
	RenderAtom grey  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	dateLabel.Init( &gamui2D );
	techLabel.Init( &gamui2D );
	moneyWidget.Init( &gamui2D );
	consoleWidget.Init( &gamui2D );

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	startGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);
	endGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);

	Vector3F delta = { 14.0f, 14.0f, 14.0f };
	Vector3F target = { (float)sim->GetWorldMap()->Width() *0.5f, 0.0f, (float)sim->GetWorldMap()->Height() * 0.5f };
	uiMode[UI_VIEW].SetDown();
	if (sim->GetPlayerChit()) {
		target = sim->GetPlayerChit()->GetSpatialComponent()->GetPosition();
	}
	else if (sim->GetChitBag()->GetHomeCore()) {
		target = sim->GetChitBag()->GetHomeCore()->ParentChit()->GetSpatialComponent()->GetPosition();
	}
	sim->GetEngine()->CameraLookAt(target + delta, target);
	sim->GetChitBag()->AddListener(this);

	for (int i = 0; i < NUM_BUILD_MARKS; ++i) {
		buildMark[i].Init(&sim->GetWorldMap()->overlay1, grey, false);
		buildMark[i].SetVisible(false);
	}

	RenderAtom redAtom = LumosGame::CalcUIIconAtom("warning");
	RenderAtom yellowAtom = LumosGame::CalcUIIconAtom("yellowwarning");
	RenderAtom nullAtom;

	coreWarningIcon.Init(&gamui2D, game->GetButtonLook(0));
	domainWarningIcon.Init(&gamui2D, game->GetButtonLook(0));
	coreWarningIcon.SetDeco(redAtom, redAtom);
	domainWarningIcon.SetDeco(yellowAtom, yellowAtom);

	coreWarningIcon.SetText("WARNING: Core under attack.");
	domainWarningIcon.SetText("WARNING: Domain under attack.");
}


GameScene::~GameScene()
{
	if ( selectionModel ) {
		sim->GetEngine()->FreeModel( selectionModel );
	}
	delete sim;
}


void GameScene::Resize()
{
	const Screenport& port = lumosGame->GetScreenport();
	
	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();

	layout.PosAbs(&censusButton, 0, -1);
	layout.PosAbs(&helpText, 1, -1);
	layout.PosAbs(&saveButton, 1, -1);
	layout.PosAbs(&loadButton, 2, -1);
	layout.PosAbs(&allRockButton, 3, -1);
	layout.PosAbs(&okay, 4, -1);

	layout.PosAbs(&useBuildingButton, 0, 2);

	layout.PosAbs(&cameraHomeButton, 0, 1);
	layout.PosAbs(&prevUnit, 1, 1);
	layout.PosAbs(&avatarUnit, 2, 1);
	layout.PosAbs(&nextUnit, 3, 1);

	int level = BuildScript::GROUP_UTILITY;
	int start = 0;

	for( int i=1; i<BuildScript::NUM_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );
		if ( bd.group != level ) {
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
//	buildButton[0].SetVisible(false);	// the "none" button. Not working - perhaps bug in sub-selection.
	buildButton[0].SetPos(-100, -100);

	for (int i = 0; i<NUM_BUILD_MODES; ++i) {
		layout.PosAbs( &modeButton[i], i, 1 );
	}
	layout.PosAbs(&createWorkerButton, 0, 3);
	layout.PosAbs(&autoRebuild, 1, 3);
	layout.PosAbs(&abandonButton, 2, 3);

	for( int i=0; i<NUM_UI_MODES; ++i ) {
		layout.PosAbs( &uiMode[i], i, 0 );
	}

	layout.PosAbs(&buildDescription, 0, 4);

	layout.PosAbs(&coreWarningIcon, 1, 6);
	layout.PosAbs(&domainWarningIcon, 1, 7);

	coreWarningIcon.SetVisible(false);
	domainWarningIcon.SetVisible(false);

	tabBar0.SetPos(  uiMode[0].X(), uiMode[0].Y() );
	tabBar0.SetSize( uiMode[NUM_UI_MODES-1].X() + uiMode[NUM_UI_MODES-1].Width() - uiMode[0].X(), uiMode[0].Height() );
	tabBar1.SetPos(  modeButton[0].X(), modeButton[0].Y() );
	tabBar1.SetSize( modeButton[NUM_BUILD_MODES-1].X() + modeButton[NUM_BUILD_MODES-1].Width() - modeButton[0].X(), modeButton[0].Height() );

	layout.PosAbs( &faceWidget, -1, 0, 1, 1 );
	layout.PosAbs( &minimap,    -2, 0, 1, 1 );
	minimap.SetSize( minimap.Width(), minimap.Width() );	// make square
	layout.PosAbs(&atlasButton, -2, 2);	// to set size and x-value
	atlasButton.SetPos(atlasButton.X(), minimap.Y() + minimap.Height());

	faceWidget.SetSize( faceWidget.Width(), faceWidget.Width() );

	layout.PosAbs( &dateLabel,   -3, 0 );
	layout.PosAbs( &moneyWidget, 5, -1 );
	techLabel.SetPos( moneyWidget.X() + moneyWidget.Width() + layout.SpacingX(),
					  moneyWidget.Y() );
	layout.PosAbs(&swapWeapons, -1, 5);

	static int CONSOLE_HEIGHT = 2;	// in layout...
	layout.PosAbs(&consoleWidget, 0, -1 - CONSOLE_HEIGHT - 1);
	float consoleHeight = okay.Y() - consoleWidget.Y();
	consoleWidget.SetBounds( 0, consoleHeight );

	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		layout.PosAbs( &pickupButton[i], 0, i+3 );
	}

	layout.PosAbs(&startGameWidget, 2, 2, 5, 5);
	layout.PosAbs(&endGameWidget, 2, 2, 5, 5);

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
	loadButton.SetVisible(visible);
	okay.SetVisible(visible);
}


void GameScene::SetBars( Chit* chit )
{
	ItemComponent* ic = chit ? chit->GetItemComponent() : 0;
	AIComponent* ai = chit ? chit->GetAIComponent() : 0;

	faceWidget.SetMeta( ic, ai );
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
	chitTracking = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;
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
	ModelVoxel mv = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, 0, 0, &at );
	MoveModel( mv.model ? mv.model->userData : 0 );

	if ( mv.model && mv.model->userData ) {
		infoID = mv.model->userData->ID();
	}
//	if ( mv.VoxelHit() ) {
//		voxelInfoID = mv.Voxel2();
//	}
	voxelInfoID = ToWorld2I(at);

	SetSelectionModel( view );
}


void GameScene::SetSelectionModel( const grinliz::Vector2F& view )
{
	Vector3F at = { 0, 0, 0 };
	ModelVoxel mv = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, 0, 0, &at );
	Vector2I pos2i = { (int)at.x, (int)at.z };

	// --- Selection display. (Only in desktop interface.)
	Engine* engine = sim->GetEngine();
	float size = 1.0f;
	int height = 1;
	const char* name = "";
	if ( buildActive ) {
		if (    buildActive == BuildScript::CLEAR 
			 || buildActive == BuildScript::CANCEL ) 
		{
			const WorldGrid& wg = sim->GetWorldMap()->GetWorldGrid( pos2i.x, pos2i.y );
			switch ( wg.Height() ) {
			case 3:	name = "clearMarker3";	break;
			case 2:	name = "clearMarker2";	break;
			default:	name = "clearMarker1";	break;
			}
		}
		else { 
			BuildScript buildScript;
			int s = buildScript.GetData( buildActive ).size;
			if ( s == 1 ) {
				name = "buildMarker1";
			}
			else {
				size = 2.0f;
				name = "buildMarker2";
			}
		}
	}
	if ( *name ) {
		// Make the model current.
		if ( !selectionModel || !StrEqual( selectionModel->GetResource()->Name(), name )) {
			if ( selectionModel ) {
				engine->FreeModel( selectionModel );
			}
			selectionModel = engine->AllocModel( name );
			GLASSERT( selectionModel );
		}
	}
	else {
		if ( selectionModel ) {
			engine->FreeModel( selectionModel );
			selectionModel = 0;
		}
	}
	if ( selectionModel ) {
		// Move away from the eye so that the new color is visible.
		Vector3F pos = at;
		pos.x = floorf( at.x ) + size*0.5f;
		pos.z = floorf( at.z ) + size*0.5f;
		
		Vector3F dir = pos - engine->camera.PosWC();
		dir.Normalize();
		pos = pos + dir*0.01f;

		selectionModel->SetPos( pos );
		Vector4F color = { 1,1,1, 0.3f };
		selectionModel->SetColor( color );
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
	Chit* player = sim->GetPlayerChit();
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

		if ( ai && Team::GetRelationship( target, player ) == RELATE_ENEMY ) {
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
	Chit* player = sim->GetPlayerChit();
	if ( !player ) {
		ClearTargetFlags();

		AIComponent* ai = target->GetAIComponent();
		if ( ai ) {
			ai->EnableDebug( true );
		}
	}

	AIComponent* ai = player ? player->GetAIComponent() : 0;
	const char* setTarget = 0;

	if ( ai && Team::GetRelationship( target, player ) == RELATE_ENEMY ) {
		ai->Target( target, true );
		setTarget = "target";
	}
	else if ( ai && Team::GetRelationship( target, player ) == RELATE_FRIEND ) {
		const GameItem* item = target->GetItem();
		bool denizen = strstr( item->ResourceName(), "human" ) != 0;
		if (denizen) {
			chitTracking = target->ID();
			//setTarget = "possibleTarget";

			CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
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


void GameScene::Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Process3DTap(action, view, world, sim->GetEngine());
	CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
	cc->SetTrack(0);
}


void GameScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, sim->GetEngine());
	CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
	cc->SetTrack(0);
}


void GameScene::Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Engine* engine = sim->GetEngine();
	WorldMap* map = sim->GetWorldMap();

	CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();

	Vector3F atModel = { 0, 0, 0 };
	Vector3F plane = { 0, 0, 0 };
	ModelVoxel mv = ModelAtMouse(view, sim->GetEngine(), TEST_HIT_AABB, 0, MODEL_CLICK_THROUGH, 0, &plane);
	Vector2I plane2i = { (int)plane.x, (int)plane.z };
	if (!map->Bounds().Contains(plane2i)) return;

	const BuildData& buildData = buildScript.GetData(buildActive);
	BuildAction(plane2i);

	if (coreScript && uiMode[UI_CONTROL].Down()) {
		coreScript->ToggleFlag(plane2i);
		return;
	}

	if (mv.VoxelHit()) {
		if (AvatarSelected()) {
			// clicked on a rock. Melt away!
			Chit* player = sim->GetPlayerChit();
			if (player && player->GetAIComponent()) {
				player->GetAIComponent()->RockBreak(mv.Voxel2());
				return;
			}
		}
	}

	Chit* playerChit = sim->GetPlayerChit();
	if (playerChit && !buildActive) {
		if (mv.model) {
			TapModel(mv.model->userData);
		}
		else {
			Vector2F dest = { plane.x, plane.z };
			DoDestTapped(dest);
		}
	}
}

bool GameScene::DragAtom(gamui::RenderAtom* atom)
{
	if (buildActive && buildActive <= BuildScript::ICE && buildActive != BuildScript::ROTATE) {
		if (buildActive == BuildScript::CLEAR || buildActive == BuildScript::CANCEL)
			*atom = lumosGame->CalcIconAtom("delete");
		else
			*atom = lumosGame->CalcIconAtom("build");
		return true;
	}
	return false;
}


void GameScene::BuildAction(const Vector2I& pos2i)
{
	CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();
	if (coreScript && buildActive) {
		WorkQueue* wq = coreScript->GetWorkQueue();
		GLASSERT(wq);
		RemovableFilter removableFilter;

		if (buildActive == BuildScript::CLEAR) {
			wq->AddAction(pos2i, BuildScript::CLEAR);
			return;
		}
		else if (buildActive == BuildScript::CANCEL) {
			wq->Remove(pos2i);
		}
		else if (buildActive == BuildScript::ROTATE) {
			Chit* chit = sim->GetChitBag()->QueryBuilding(pos2i);
			int circuit = sim->GetWorldMap()->Circuit(pos2i.x, pos2i.y);

			if (chit) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				if (msc && msc->Building()) {
					float r = msc->GetYRotation();
					r += 90.0f;
					r = NormalizeAngleDegrees(r);
					msc->SetYRotation(r);
				}
			}
			else if (circuit) {
				sim->GetWorldMap()->SetCircuitRotation(pos2i.x, pos2i.y, sim->GetWorldMap()->CircuitRotation(pos2i.x, pos2i.y) + 1);
				sim->GetChitBag()->Context()->circuitSim->EtchLines(ToSector(pos2i));
			}
		}
		else {
			wq->AddAction(pos2i, buildActive);
		}
	}
}


void GameScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	tapView = view;	// justs a temporary to pass through to ItemTapped()
	bool uiHasTap = ProcessTap(action, view, world);
	Engine* engine = sim->GetEngine();
	WorldMap* map = sim->GetWorldMap();
	Vector3F at = { 0, 0, 0 };
	IntersectRayAAPlane(world.origin, world.direction, 1, 0, &at, 0);

	RenderAtom atom;
	if (!uiHasTap) {
		if (action == GAME_TAP_DOWN) {
			mapDragStart.Zero();
			if (DragAtom(&atom)) {
				mapDragStart.Set(at.x, at.z);
			}
		}
		if (action == GAME_TAP_MOVE) {
			if (!mapDragStart.IsZero()) {
				Vector2F end = { at.x, at.z };
				int count = 0;
				if ((end - mapDragStart).LengthSquared() > 0.25f) {
					Rectangle2I r;
					r.FromPair(ToWorld2I(mapDragStart), ToWorld2I(end));
					DragAtom(&atom);

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
		}

		if (action == GAME_TAP_UP) {
			if (mapDragStart.IsZero()) {
				Tap3D(view, world);
			}
			else {
				Vector2F end = { at.x, at.z };
				//if ((end - mapDragStart).LengthSquared() > 0.25f) {
					Rectangle2I r;
					r.FromPair(ToWorld2I(mapDragStart), ToWorld2I(end));
					DragAtom(&atom);

					for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
						BuildAction(it.Pos());
					}
				//}
			}
		}
	}
	// Clear out drag indicator, if not dragging.
	if (uiHasTap || action == GAME_TAP_UP) {
		for (int i = 0; i < NUM_BUILD_MARKS; ++i) {
			buildMark[i].SetVisible(false);
		}
		mapDragStart.Zero();
	}
	if (!uiHasTap && action == GAME_TAP_DOWN && gamui2D.DisabledTapCaptured()) {
		for (int i = 1; i < BuildScript::NUM_OPTIONS; ++i) {
			if (&buildButton[i] == gamui2D.DisabledTapCaptured()) {
				BuildScript buildScript;
				const BuildData& data = buildScript.GetData(i);
				buildDescription.SetText(data.requirementDesc ? data.requirementDesc : "");
			}
		}
	}
}


bool GameScene::CameraTrackingAvatar()
{
	Chit* playerChit = sim->GetPlayerChit();
	CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
	if (playerChit && cc && (playerChit->ID() == cc->Tracking())) {
		return true;
	}
	return false;
}


bool GameScene::AvatarSelected()
{
	bool button = uiMode[UI_VIEW].Down();
	Chit* playerChit = sim->GetPlayerChit();
	if (button && playerChit && playerChit->ID() == chitTracking) {
		return true;
	}
	return false;
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	Vector2F dest = { 0, 0 };

	if (gamui2D.DialogDisplayed(startGameWidget.Name())) {
		startGameWidget.ItemTapped(item);
	}
	else if (gamui2D.DialogDisplayed(endGameWidget.Name())) {
		endGameWidget.ItemTapped(item);
	}
	else if ( item == &okay ) {
		game->PopScene();
	}
	else if (item == &minimap) {
		float x = 0, y = 0;
		gamui2D.GetRelativeTap(&x, &y);
		GLOUTPUT(("minimap tapped nx=%.1f ny=%.1f\n", x, y));

		Engine* engine = sim->GetEngine();
		x *= float(engine->GetMap()->Width());
		y *= float(engine->GetMap()->Height());
		CameraComponent* cc = sim->GetChitBag()->GetCamera(engine);
		cc->SetTrack(0);
		engine->CameraLookAt(x, y);
	}
	else if (item == &atlasButton) {
		MapSceneData* data = new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), sim->GetPlayerChit() );
		Vector3F at;
		sim->GetEngine()->CameraLookingAt(&at);
		at.x = Clamp(at.x, 0.0f, sim->GetWorldMap()->Width() - 1.0f);
		at.z = Clamp(at.z, 0.0f, sim->GetWorldMap()->Height() - 1.0f);

		data->destSector = ToSector(ToWorld2I(at));
		game->PushScene( LumosGame::SCENE_MAP, data );
	}
	else if ( item == &saveButton ) {
		Save();
	}
	else if ( item == &loadButton ) {
		delete sim;
		sim = new Sim( lumosGame );
		Load();
	}
	else if ( item == &allRockButton ) {
		sim->SetAllRock();
	}
	else if ( item == &censusButton ) {
		game->PushScene( LumosGame::SCENE_CENSUS, new CensusSceneData( sim->GetChitBag()) );
	}
	else if ( item == &createWorkerButton ) {
		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
		if (cs) {
			Chit* coreChit = cs->ParentChit();
			if (coreChit->GetItem()->wallet.gold >= WORKER_BOT_COST) {
				Transfer(&ReserveBank::Instance()->bank, &coreChit->GetItem()->wallet, WORKER_BOT_COST);
				int team = coreChit->GetItem()->team;
				sim->GetChitBag()->NewWorkerChit(coreChit->GetSpatialComponent()->GetPosition(), team);
			}
		}
	}
	else if (item == &autoRebuild) {
		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
		if (cs) {
			Chit* coreChit = cs->ParentChit();
			Component* rebuild = coreChit->GetComponent("RebuildAIComponent");

			if (autoRebuild.Down() && !rebuild) {
				coreChit->Add(new RebuildAIComponent());
			}
			else if (!autoRebuild.Down() && rebuild) {
				coreChit->Remove(rebuild);
				delete rebuild;
			}
		}
	}
	else if (item == &abandonButton) {
		// Add a rebuild, if not present, and then abandon.
		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
		if (cs) {
			Chit* coreChit = cs->ParentChit();
			Component* rebuild = coreChit->GetComponent("RebuildAIComponent");
			if (!rebuild) {
				coreChit->Add(new RebuildAIComponent());
			}
		}
		sim->AbandonDomain();
	}
	else if ( item == faceWidget.GetButton() ) {
		Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
		if (!chit) {
			chit = sim->GetPlayerChit();
		}
		if ( chit && chit->GetItemComponent() ) {			
			game->PushScene( LumosGame::SCENE_CHARACTER, 
							 new CharacterSceneData( chit->GetItemComponent(), 0, 
							 chit == sim->GetPlayerChit() ? CharacterSceneData::AVATAR : CharacterSceneData::CHARACTER_ITEM, 
							 0 ));
		}
	}
	else if (item == &swapWeapons) {
		Chit* player = sim->GetPlayerChit();
		if (player && player->GetItemComponent()) {
			player->GetItemComponent()->SwapWeapons();
		}
	}
	else if ( item == &useBuildingButton ) {
		sim->UseBuilding();
	}
	else if ( item == &cameraHomeButton ) {
		CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();
		if ( coreScript ) {
			Chit* chit = coreScript->ParentChit();
			if ( chit && chit->GetSpatialComponent() ) {
				Vector3F lookAt = chit->GetSpatialComponent()->GetPosition();
				sim->GetEngine()->CameraLookAt( lookAt.x, lookAt.z );
				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
				cc->SetTrack(0);
			}
		}
	}
	else if ( item == &prevUnit || item == &nextUnit || item == &avatarUnit ) {
		CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();

		if (item == &avatarUnit && AvatarSelected() && CameraTrackingAvatar()) {
			if (coreScript && sim->GetPlayerChit()) {
				sim->GetPlayerChit()->GetSpatialComponent()->Teleport(coreScript->ParentChit()->GetSpatialComponent()->GetPosition());
			}
		}
		else {
			int bias = 0;
			if (item == &prevUnit) bias = -1;
			if (item == &nextUnit) bias = 1;

			if (coreScript && coreScript->NumCitizens()) {
				if (item == &avatarUnit) {
					chitTracking = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;
				}
				Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
				int index = 0;
				if (chit) {
					index = coreScript->FindCitizenIndex(chit);
				}

				if (index < 0)
					index = 0;
				else
					index = index + bias;

				if (index < 0) index += coreScript->NumCitizens();
				if (index >= coreScript->NumCitizens()) index = 0;

				chit = coreScript->CitizenAtIndex(index);

				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
				if (cc && chit) {
					chitTracking = chit->ID();
					cc->SetTrack(chitTracking);
				}
			}
		}
	}
	else if (item == &coreWarningIcon) {
		CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
		if (cc) {
			cc->SetPanTo(coreWarningPos);
		}
	}
	else if (item == &domainWarningIcon) {
		CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
		if (cc) {
			cc->SetPanTo(domainWarningPos);
		}
	}

	Vector2F pos2 = { 0, 0 };
	if (consoleWidget.IsItem(item, &pos2)) {
		if (!pos2.IsZero()) {
			CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
			if (cc) {
				cc->SetPanTo(pos2);
			}
		}
	}

	// Only hitting the bottom row (actual action) buttons triggers
	// the build. Up until that time, the selection icon doesn't 
	// turn on.
	buildActive = 0;
	buildDescription.SetText("");
	if (uiMode[UI_BUILD].Down()) {
		for (int i = 1; i < BuildScript::NUM_OPTIONS; ++i) {
			if (&buildButton[i] == item) {
				buildActive = i;
				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
				cc->SetTrack(0);

				BuildScript buildScript;
				const BuildData& bd = buildScript.GetData(i);
				buildDescription.SetText(bd.desc ? bd.desc : "");

				break;
			}
		}
	}
	SetSelectionModel(tapView);

	// If a mode switches, set the buttons up so there isn't a down
	// button with nothing being built.
	bool setBuildUp = false;
	for (int i = 0; i < NUM_BUILD_MODES; ++i) {
		if (item == &modeButton[i]) 
			setBuildUp = true;
	}
	if (item == &uiMode[UI_BUILD]) {
		setBuildUp = true;
	}
	if (setBuildUp) {
		buildButton[0].SetDown();
	}

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		if (item == &newsButton[i]) {
			NewsHistory* history = sim->GetChitBag()->GetNewsHistory();
			const grinliz::CArray< NewsEvent, NewsHistory::MAX_CURRENT >& current = history->CurrentNews();

			int index = current.Size() - 1 - i;
			if (index >= 0) {
				const NewsEvent& ne = current[index];
				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
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
	if ( item == &clearButton ) {
		if ( FreeCameraMode() ) {
			CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
			if ( cc  ) {
				cc->SetTrack( 0 );
			}
		}
	}
#endif

	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		if ( item == &pickupButton[i] ) {
			Chit* playerChit = sim->GetPlayerChit();
			Chit* item = sim->GetChitBag()->GetChit( pickupData[i].chitID );
			if ( item && playerChit ) {
				if ( playerChit->GetAIComponent() ) {
					playerChit->GetAIComponent()->Pickup( item );
				}
			}
		}
	}

	if ( !dest.IsZero() ) {
		DoDestTapped( dest );
	}
}


void GameScene::DoDestTapped( const Vector2F& _dest )
{
	Vector2F dest = _dest;

	Chit* chit = sim->GetPlayerChit();
	if (chit) {
		AIComponent* ai = chit->GetAIComponent();
		if ( ai ) {
			Vector2F pos = chit->GetSpatialComponent()->GetPosition2D();
			// Is this grid travel or normal travel?
			Vector2I currentSector = SectorData::SectorID( pos.x, pos.y );
			Vector2I destSector    = SectorData::SectorID( dest.x, dest.y );
			SectorPort sectorPort;

			if ( currentSector != destSector )
			{
				// Find the nearest port. (Somewhat arbitrary.)
				sectorPort.sector = destSector;
				sectorPort.port   = sim->GetWorldMap()->GetSector( sectorPort.sector ).NearestPort( pos );
			}
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
	if ( mask == GAME_HK_TOGGLE_FAST ) {
		fastMode = !fastMode;
	}
	else if (mask == GAME_HK_ESCAPE) {
		if (buildActive > 0) {
			// back out of build
			buildActive = 0;
			buildButton[0].SetDown();
			SetSelectionModel(tapView);
		}
		else if (uiMode[UI_BUILD].Down() || uiMode[UI_CONTROL].Down()) {
			// return to view
			uiMode[UI_VIEW].SetDown();
		}
		buildDescription.SetText("");
	}
	else if (mask == GAME_HK_CAMERA_AVATAR) {
		CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();
		if (coreScript && sim->GetPlayerChit()) {
			if (buildActive > 0) {
				// back out of build
				buildActive = 0;
				buildButton[0].SetDown();
				SetSelectionModel(tapView);
			}
			if (AvatarSelected() && CameraTrackingAvatar()) {
				sim->GetPlayerChit()->GetSpatialComponent()->Teleport(coreScript->ParentChit()->GetSpatialComponent()->GetPosition());
			}
			CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
			if (cc) {
				cc->SetTrack(sim->GetPlayerChit()->ID());
				uiMode[UI_VIEW].SetDown();
			}
		}
	}
	else if (mask == GAME_HK_CAMERA_CORE) {
		CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();
		CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
		if (cc) {
			cc->SetTrack(coreScript->ParentChit()->ID());
		}
	}
	else if (mask == GAME_HK_SPACE) {
		Chit* playerChit = sim->GetPlayerChit();
#ifdef DEBUG
#if 0
		if ( playerChit ) {
			ItemComponent* ic = playerChit->GetItemComponent();
			if ( ic ) {
				ic->SwapWeapons();
			}
		}
#endif
#if 0
		if ( playerChit ) {
			AIComponent* ai = playerChit->GetAIComponent();
			ai->Rampage( 0 );
		}
#endif
		Vector3F at = V3F_ZERO;
		sim->GetEngine()->CameraLookingAt(&at);
#if 1
		for (int i = 0; i<5; ++i) {
			//sim->GetChitBag()->NewMonsterChit(plane, "redMantis", TEAM_RED_MANTIS);
			sim->GetChitBag()->NewMonsterChit(at, "mantis", TEAM_GREEN_MANTIS);
			at.x += 0.5f;
		}
#endif
#if 0
		for (int i = 0; i < NUM_PLANT_TYPES; ++i) {
			Chit* chit = sim->CreatePlant((int)at.x + i, (int)at.z, i);
			if (chit) {
				if (i < 6) {
					PlantScript* plantScript = (PlantScript*)chit->GetScript("PlantScript");
					GLASSERT(plantScript);
					plantScript->SetStage(3);
				}
			}
		}
#endif
#if 0
		Vector2I loc = sim->GetWorldMap()->GetPoolLocation(poolView);
		if (loc.IsZero()) {
			poolView = 0;
		}
		++poolView;
		Vector2F lookAt = ToWorld2F(loc);
		sim->GetEngine()->CameraLookAt(lookAt.x, lookAt.y);
#endif
#if 0
		if (playerChit) {
			sim->GetChitBag()->AddSummoning(playerChit->GetSpatialComponent()->GetSector(), LumosChitBag::SUMMON_TECH);
		}
#endif
#endif	// DEBUG
	}
	else if (mask == GAME_HK_CAMERA_TOGGLE) {
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
	else if ( mask == GAME_HK_TOGGLE_COLORS ) {
		static int colorSeed = 0;
		const IString NAMES[4] = { 
			IStringConst::kiosk__m, 
			IStringConst::kiosk__n, 
			IStringConst::kiosk__c, 
			IStringConst::kiosk__s 
		};
		ItemNameFilter filter( NAMES, 4, IChitAccept::MAP );
		Vector3F at;
		sim->GetEngine()->CameraLookingAt( &at );
		Vector2F at2 = { at.x, at.z };
		CChitArray queryArr;
		sim->GetChitBag()->QuerySpatialHash( &queryArr, at2, 10.f, 0, &filter );
		for( int i=0; i<queryArr.Size(); ++i ) {
			RenderComponent* rc = queryArr[i]->GetRenderComponent();
			TeamGen gen;
			ProcRenderInfo info;
			gen.Assign( colorSeed, &info );
			rc->SetProcedural( 0, info );
		}
		++colorSeed;
	}
	else if ( mask == GAME_HK_TOGGLE_PATHING ) {
		Rectangle2I r;
		if ( sim->GetWorldMap()->IsShowingRegionOverlay() ) {
			r.Set( 0, 0, 0, 0 );
		}
		else {
			Vector3F at;
			sim->GetEngine()->CameraLookingAt( &at );
			Vector2I sector = { (int)at.x / SECTOR_SIZE, (int)at.z / SECTOR_SIZE };
			r.Set( sector.x*SECTOR_SIZE, sector.y*SECTOR_SIZE, (sector.x+1)*SECTOR_SIZE-1, (sector.y+1)*SECTOR_SIZE-1 );
		}
		sim->GetWorldMap()->ShowRegionOverlay( r );
	}
	else if ( mask == GAME_HK_MAP ) {
		Chit* playerChit = sim->GetPlayerChit();
		game->PushScene( LumosGame::SCENE_MAP, new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), playerChit ));
	}
	else if ( mask == GAME_HK_CHEAT_GOLD ) {
		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
		if ( cs ) {
			static const int GOLD = 100;
			ReserveBank::Instance()->bank.AddGold( -GOLD );
			cs->ParentChit()->GetItem()->wallet.AddGold( GOLD );
		}
	}
	else if (mask == GAME_HK_CHEAT_ELIXIR) {
//		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
//		if (cs) {
//			cs->nElixir += 20;
//		}
	}
	else if ( mask == GAME_HK_CHEAT_CRYSTAL ) {
		CoreScript* cs = sim->GetChitBag()->GetHomeCore();
		if ( cs ) {
			for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
				if ( ReserveBank::Instance()->bank.crystal[i] ) {
					ReserveBank::Instance()->bank.AddCrystal( i, -1 );
					cs->ParentChit()->GetItem()->wallet.AddCrystal(i, 1 );
				}
			}
		}
	}
	else if ( mask == GAME_HK_CHEAT_TECH ) {
		Chit* player = sim->GetPlayerChit();

		CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();
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
		ForceHerd(sector);
	}
	else {
		super::HandleHotKey( mask );
	}
}


void GameScene::ForceHerd(const grinliz::Vector2I& sector)
{
	CDynArray<Chit*> arr;
	MOBKeyFilter filter;
	Rectangle2F bounds = ToWorld(InnerSectorBounds(sector));
	sim->GetChitBag()->QuerySpatialHash(&arr, bounds, 0, &filter);

	for (int i = 0; i<arr.Size(); ++i) {
		IString mob = arr[i]->GetItem()->keyValues.GetIString(ISC::mob);
		if (mob == ISC::lesser || mob == ISC::greater) {

			/*
			// Move the MOB to the port first, if possible, so they don't 
			// turn around and attack the new core.
			PathMoveComponent* pmc = GET_SUB_COMPONENT(arr[i], MoveComponent, PathMoveComponent);
			if (pmc) {
				pmc->Stop();
				SpatialComponent* sc = arr[i]->GetSpatialComponent();
				GLASSERT(sc);
				const SectorData& sd = sim->GetWorldMap()->GetSector(sector);
				
				int port = sd.NearestPort(sc->GetPosition2D());
				if (port) {
					Vector2F v = SectorData::PortPos(sd.GetPortLoc(port), arr[i]->ID());
					sc->SetPosition(v.x, 0, v.y);
				}
			}
			*/
			AIComponent* ai = arr[i]->GetAIComponent();
			ai->GoSectorHerd(true);
		}
	}
}


void GameScene::SetHelpText(const int* arr, int nWorkers)
{
	static const int BUILD_ADVISOR[] = {
		BuildScript::FARM,
		BuildScript::SLEEPTUBE,
		BuildScript::DISTILLERY,
		BuildScript::BAR,
		BuildScript::MARKET,
		BuildScript::FORGE,
		BuildScript::TEMPLE,
		BuildScript::GUARDPOST,
		BuildScript::EXCHANGE,
		BuildScript::KIOSK_N,
		BuildScript::KIOSK_M,
		BuildScript::KIOSK_C,
		BuildScript::KIOSK_S,
		BuildScript::VAULT,
		BuildScript::CIRCUITFAB
	};

	static const int NUM_BUILD_ADVISORS = GL_C_ARRAY_SIZE(BUILD_ADVISOR);
	CoreScript* cs = sim->GetChitBag()->GetHomeCore();
	CStr<100> str = "";

	if (cs) {
		if (nWorkers == 0) {
			str.Format("Build a worker bot.");
		}
		else {
			const Wallet& wallet = cs->ParentChit()->GetItem()->wallet;

			for (int i = 0; i < NUM_BUILD_ADVISORS; ++i) {
				int id = BUILD_ADVISOR[i];
				if (arr[id] == 0) {
					BuildScript buildScript;
					const BuildData& data = buildScript.GetData(id);

					if (wallet.gold >= data.cost) {
						str.Format("Advisor: Build a %s.", data.cName);
					}
					else {
						str.Format("Advisor: Collect gold by defeating attackers\nor raiding domains.");
					}
					break;
				}
			}
		}
	}
	helpText.SetText(str.c_str());
}


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
	int maxTubes  = 4 << techLevel;

	BuildScript buildScript;
	const BuildData* sleepTubeData = buildScript.GetDataFromStructure(IStringConst::bed, 0);
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
	buildButton[BuildScript::KIOSK_N].SetEnabled(nTemples > 0);
	buildButton[BuildScript::KIOSK_C].SetEnabled(nTemples > 0);
	buildButton[BuildScript::KIOSK_S].SetEnabled(nTemples > 0);
	buildButton[BuildScript::KIOSK_M].SetEnabled(nTemples > 0);

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

void GameScene::SetPickupButtons()
{
	Chit* player = sim->GetPlayerChit();
	if ( AvatarSelected() ) {
		bool canAdd = player && player->GetItemComponent() && player->GetItemComponent()->CanAddToInventory();
		// Query items on the ground in a radius of the player.
		LootFilter lootFilter;
		static const float LOOT_RAD = 10.0f;
		Vector2F pos = player->GetSpatialComponent()->GetPosition2D();

		sim->GetChitBag()->QuerySpatialHash( &chitQuery, 
											 pos, LOOT_RAD,
											 0, &lootFilter );
		
		// Remove things that aren't pathable.
		pickupData.Clear();
		for( int i=0; i<chitQuery.Size(); i++ ) {
			float cost = 0;
			bool hasPath = sim->GetWorldMap()->CalcPath( pos, chitQuery[i]->GetSpatialComponent()->GetPosition2D(),
														 0, &cost, false );
			if ( hasPath ) {
				PickupData pd = { chitQuery[i]->ID(), cost };
				pickupData.Push( pd );
			}
		}

		// Sort near to far.
		Sort< PickupData, CompValue >( pickupData.Mem(), pickupData.Size() );

		int i=0;
		for( ; i<pickupData.Size() && i < NUM_PICKUP_BUTTONS; ++i ) {
			Chit* chit = sim->GetChitBag()->GetChit( pickupData[i].chitID );
			if ( chit && chit->GetItem() ) {
				pickupButton[i].SetVisible( true );
				pickupButton[i].SetEnabled( canAdd );
				lumosGame->ItemToButton( chit->GetItem(), &pickupButton[i] );
			}
		}
		for( ; i < NUM_PICKUP_BUTTONS; ++i ) {
			pickupButton[i].SetVisible( false );
		}
	}
	else {
		for( int i=0; i<NUM_PICKUP_BUTTONS; ++i )
			pickupButton[i].SetVisible( false );
	}
}


void GameScene::ProcessNewsToConsole()
{
	NewsHistory* history = sim->GetChitBag()->GetNewsHistory();
	currentNews = Max( currentNews, history->NumNews() - 40 );
	GLString str;
	LumosChitBag* chitBag = sim->GetChitBag();
	CoreScript* coreScript = chitBag->GetHomeCore();

	Vector2I avatarSector = { 0, 0 };
	Chit* playerChit = sim->GetPlayerChit();
	if ( playerChit && playerChit->GetSpatialComponent() ) {
		avatarSector = ToSector( playerChit->GetSpatialComponent()->GetPosition2DI() );
	}
	Vector2I homeSector = sim->GetChitBag()->GetHomeSector();

	// Check if news sector is 1)current avatar sector, or 2)domain sector

	RenderAtom atom;
	Vector2F pos2 = { 0, 0 };

	for ( ;currentNews < history->NumNews(); ++currentNews ) {
		const NewsEvent& ne = history->News( currentNews );
		Vector2I sector = ToSector(ToWorld2I(ne.pos));
		Chit* chit = chitBag->GetChit(ne.chitID);
		SpatialComponent* sc = chit ? chit->GetSpatialComponent() : 0;

		str = "";

		switch( ne.what ) {
		case NewsEvent::DENIZEN_CREATED:
			if ( coreScript && coreScript->IsCitizen( ne.chitID )) {
				ne.Console( &str, chitBag, 0 );
				if (sc) pos2 = sc->GetPosition2D();
				atom = LumosGame::CalcUIIconAtom("greeninfo");
			}
			break;

		case NewsEvent::DENIZEN_KILLED:
		case NewsEvent::STARVATION:
		case NewsEvent::BLOOD_RAGE:
		case NewsEvent::VISION_QUEST:
			if ( coreScript && coreScript->IsCitizen( ne.chitID )) {
				ne.Console( &str, chitBag, 0 );
				if (sc) pos2 = sc->GetPosition2D();
				atom = LumosGame::CalcUIIconAtom("warning");
			}
			break;

		case NewsEvent::FORGED:
		case NewsEvent::UN_FORGED:
		case NewsEvent::PURCHASED:
			if ((coreScript && coreScript->IsCitizen(ne.chitID))
				|| sector == homeSector)
			{
				ne.Console(&str, chitBag, 0);
				if (sc) pos2 = sc->GetPosition2D();
				atom = LumosGame::CalcUIIconAtom("greeninfo");
			}
			break;

		case NewsEvent::GREATER_MOB_CREATED:
		case NewsEvent::GREATER_MOB_KILLED:
			ne.Console( &str, chitBag, 0 );
			if (sc) pos2 = sc->GetPosition2D();
			atom = LumosGame::CalcUIIconAtom("greeninfo");
			break;

		case NewsEvent::DOMAIN_CREATED:
		case NewsEvent::DOMAIN_DESTROYED:
		case NewsEvent::GREATER_SUMMON_TECH:
			ne.Console( &str, chitBag, 0 );
			break;

		default:
			break;
		}
		if ( !str.empty() ) {
			//consoleWidget.Push( str );
			consoleWidget.Push(str, atom, pos2);
		}
	}
}


void GameScene::DoTick( U32 delta )
{
	clock_t startTime = clock();

	sim->DoTick( delta );
	++simCount;

	if ( fastMode ) {
		while( clock() - startTime < 100 ) {
			sim->DoTick( 30 );
			++simCount;
		}
	}

	SetPickupButtons();

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		NewsHistory* history = sim->GetChitBag()->GetNewsHistory();
		const grinliz::CArray< NewsEvent, NewsHistory::MAX_CURRENT >& current = history->CurrentNews();

		int index = current.Size() - 1 - i;
		if ( index >= 0 ) {
			const NewsEvent& ne = current[index];
			IString name = ne.GetWhat();
			newsButton[i].SetText( name.c_str() );
			newsButton[i].SetEnabled( true );
		}
		else {
			newsButton[i].SetText( "" );
			newsButton[i].SetEnabled( false );
		}
	}

	clock_t endTime = clock();
	simTimer += (int)(endTime-startTime);

	Vector3F lookAt = { 0, 0, 0 };
	sim->GetEngine()->CameraLookingAt( &lookAt );
	Vector2I viewingSector = { (int)lookAt.x / SECTOR_SIZE, (int)lookAt.z / SECTOR_SIZE };
	const SectorData& sd = sim->GetWorldMap()->GetWorldInfo().GetSector( viewingSector );

	CStr<64> str;
	str.Format( "Date %.2f\n%s", sim->AgeF(), sd.name.c_str() );
	dateLabel.SetText( str.c_str() );

	CoreScript* coreScript = sim->GetChitBag()->GetHomeCore();

	// Set the states: VIEW, BUILD, AVATAR. Avatar is 
	// disabled if there isn't one...
	Chit* playerChit = sim->GetPlayerChit();
	if ( !playerChit && !coreScript ) {
		uiMode[UI_VIEW].SetDown();
	}
	uiMode[UI_BUILD].SetEnabled(coreScript != 0);

	Chit* track = sim->GetChitBag()->GetChit( chitTracking );
	if (!track && sim->GetPlayerChit()) {
		track = sim->GetPlayerChit();
	}
	faceWidget.SetFace( &uiRenderer, track ? track->GetItem() : 0 );
	SetBars( track );
	
	// This doesn't really work. The AI will swap weapons
	// at will, so it's more frustrating than useful.
	//swapWeapons.SetVisible(track && track == playerChit);
	swapWeapons.SetVisible(false);
	
	str.Clear();

	Wallet wallet;
	CoreScript* cs = sim->GetChitBag()->GetHomeCore();
	if ( cs ) {
		wallet = cs->ParentChit()->GetItem()->wallet;
	}
	moneyWidget.Set( wallet );

	str.Clear();
	if ( playerChit && playerChit->GetItem() ) {
		const GameTrait& stat = playerChit->GetItem()->Traits();
		str.Format( "Level %d XP %d/%d", stat.Level(), stat.Experience(), GameTrait::LevelToExperience( stat.Level()+1) );
	}

	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		modeButton[i].SetVisible( uiMode[UI_BUILD].Down() );
	}
	tabBar1.SetVisible( uiMode[UI_BUILD].Down() );
	static const int CIRCUIT_MODE = NUM_BUILD_MODES - 1;
	createWorkerButton.SetVisible( uiMode[UI_BUILD].Down() && !modeButton[CIRCUIT_MODE].Down() );

	bool visible = game->GetDebugUI();
	autoRebuild.SetVisible(uiMode[UI_BUILD].Down() && visible && !modeButton[CIRCUIT_MODE].Down());
	abandonButton.SetVisible(uiMode[UI_BUILD].Down() && visible && !modeButton[CIRCUIT_MODE].Down());

	str.Clear();

	if (coreScript) {
		float tech = coreScript->GetTech();
		int maxTech = coreScript->MaxTech();
		str.Format("Tech %.2f / %d", tech, maxTech);
		techLabel.SetText(str.c_str());
	}

	CheckGameStage(delta);
	int nWorkers = 0;

	Vector2I homeSector = sim->GetChitBag()->GetHomeSector();
	{
		// Enforce the worker limit.
		CStr<32> str2;
		static const int MAX_BOTS = 4;

		Rectangle2F b = ToWorld( InnerSectorBounds(homeSector));
		CChitArray arr;
		ItemNameFilter workerFilter(IStringConst::worker, IChitAccept::MOB);
		sim->GetChitBag()->QuerySpatialHash( &arr, b, 0, &workerFilter );
		nWorkers = arr.Size();

		str2.Format("WorkerBot\n%d %d/%d", WORKER_BOT_COST, nWorkers, MAX_BOTS);		// FIXME: pull price from data
		createWorkerButton.SetText( str2.c_str() );
		createWorkerButton.SetEnabled( arr.Size() < MAX_BOTS );
	}

	{
		LumosChitBag* cb = sim->GetChitBag();
		Vector2I sector = cb->GetHomeSector();
		int arr[BuildScript::NUM_OPTIONS] = { 0 };
		cb->BuildingCounts(sector, arr, BuildScript::NUM_OPTIONS);
		SetBuildButtons(arr);
		SetHelpText(arr, nWorkers);
	}

	autoRebuild.SetEnabled(coreScript != 0);
	abandonButton.SetEnabled(coreScript != 0);
	if (coreScript) {
		// auto rebuild
		Component* rebuild = coreScript->ParentChit()->GetComponent("RebuildAIComponent");
		if (rebuild)
			autoRebuild.SetDown();
		else
			autoRebuild.SetUp();
	}

	consoleWidget.DoTick( delta );
	ProcessNewsToConsole();

	bool useBuildingVisible = false;
	if ( AvatarSelected() ) {
		Chit* building = sim->GetChitBag()->QueryPorch( playerChit->GetSpatialComponent()->GetPosition2DI(),0 );
		if ( building ) {
			IString name = building->GetItem()->IName();
			if ( name == ISC::vault || name == ISC::factory || name == ISC::market || name == ISC::exchange ) {
				useBuildingVisible = true;
			}
		}
	}
	useBuildingButton.SetVisible( useBuildingVisible );
	cameraHomeButton.SetVisible( uiMode[UI_VIEW].Down() );
	nextUnit.SetVisible( uiMode[UI_VIEW].Down() );
	prevUnit.SetVisible(uiMode[UI_VIEW].Down());
	avatarUnit.SetVisible(uiMode[UI_VIEW].Down());

	if (AvatarSelected() && CameraTrackingAvatar()) {
		avatarUnit.SetText("Teleport\nAvatar");
	}
	else {
		avatarUnit.SetText("Avatar");
	}

	sim->GetEngine()->RestrictCamera( 0 );

	// The game will open scenes - say the CharacterScene for the
	// Vault - in response to player actions. Look for them here.
	if ( !game->IsScenePushed() ) {
		int id=0;
		SceneData* data=0;
		if ( sim->GetChitBag()->PopScene( &id, &data )) {
			game->PushScene( id, data );
		}
	}

	coreWarningIcon.SetVisible(coreWarningTimer > 0);
	coreWarningTimer -= delta;

	domainWarningIcon.SetVisible(domainWarningTimer > 0);
	domainWarningTimer -= delta;
}



void GameScene::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_DESTROYED_START) {
		if (chit->GetComponent("CoreScript")) {
			if (sim->GetChitBag()->GetHomeTeam() && (chit->Team() == sim->GetChitBag()->GetHomeTeam())) {
				CoreScript* cs = (CoreScript*) chit->GetComponent("CoreScript");
				GLASSERT(cs);
				Vector2I sector = ToSector(cs->ParentChit()->GetSpatialComponent()->GetPosition2DI());
				const SectorData& sd = sim->GetWorldMap()->GetSector(sector);
				endGameWidget.SetData(	chit->Context()->chitBag->GetNewsHistory(),
										this, 
										sd.name, chit->GetItem()->ID(), 
										cs->GetAchievement());
				endTimer = 8*1000;
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
				coreWarningPos = chit->GetSpatialComponent()->GetPosition2D();
			}
			else {
				domainWarningTimer = 6000;
				domainWarningPos = chit->GetSpatialComponent()->GetPosition2D();
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
	else if (!cs && !startVisible && !endVisible) {
		// Try to find a suitable starting location.
		Rectangle2I b;
		Random random;
		random.SetSeedFromTime();

		b.min.y = b.min.x = (NUM_SECTORS / 2) - NUM_SECTORS / 4;
		b.max.y = b.max.x = (NUM_SECTORS / 2) + NUM_SECTORS / 4;

		CArray<SectorInfo, NUM_SECTORS * NUM_SECTORS> arr;
		for (Rectangle2IIterator it(b); !it.Done() && arr.HasCap(); it.Next()) {
			const SectorData* sd = &sim->GetWorldMap()->GetSector(it.Pos());
			if (sd->HasCore() && arr.HasCap()) {
				Vector2I sector = ToSector(sd->x, sd->y);
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
			Sort<SectorInfo, grinliz::CompValue>(arr.Mem(), arr.Size());
			// Randomize the first 16 - it isn't all about bioflora
			random.ShuffleArray(arr.Mem(), Min(arr.Size(), 16));

			// FIXME all needlessly complicated
			CArray<const SectorData*, 16> sdarr;
			for (int i = 0; i < arr.Size() && sdarr.HasCap(); ++i) {
				sdarr.Push(arr[i].sectorData);
			}

			startGameWidget.SetSectorData(sdarr.Mem(), sdarr.Size(), sim->GetEngine(), sim->GetChitBag(), this, sim->GetWorldMap());
			gamui2D.PushDialog(startGameWidget.Name());

			CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
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
		int team = Team::GenTeam(TEAM_HOUSE);
		sim->GetChitBag()->SetHomeTeam(team);
		sim->CreateCore(ToSector(sd->x, sd->y), team);
		ForceHerd(ToSector(sd->x, sd->y));

		ReserveBank* bank = ReserveBank::Instance();
		if (bank) {
			CoreScript* cs = CoreScript::GetCore(ToSector(sd->x, sd->y));
			GLASSERT(cs);
			Chit* parent = cs->ParentChit();
			GLASSERT(parent);
			GameItem* item = parent->GetItem();
			GLASSERT(item);
			Transfer(&item->wallet, &bank->bank, 250);
		}
	}
	gamui2D.PopDialog();
}


void GameScene::Draw3D( U32 deltaTime )
{
	sim->Draw3D( deltaTime );

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
	DrawDebugTextDrawCalls( x, y, sim->GetEngine() );
	y += 16;

	UFOText* ufoText = UFOText::Instance();
	Chit* chit = sim->GetPlayerChit();
	Engine* engine = sim->GetEngine();
	LumosChitBag* chitBag = sim->GetChitBag();
	Vector3F at = { 0, 0, 0 };
	WorldMap* worldMap = sim->GetWorldMap();

	if ( chit && chit->GetSpatialComponent() ) {
		const Vector3F& v = chit->GetSpatialComponent()->GetPosition();
		at = v;
		ufoText->Draw( x, y, "Player: %.1f, %.1f, %.1f  Camera: %.1f %.1f %.1f", 
			           v.x, v.y, v.z,
					   engine->camera.PosWC().x, engine->camera.PosWC().y, engine->camera.PosWC().z );
		y += 16;
	}
	else {
		engine->CameraLookingAt( &at );
	}

	if ( simTimer > 1000 ) {
		simPS = 1000.0f * (float)simCount / (float)simTimer;
		simTimer = 0;
		simCount = 0;
	}

	Wallet w = ReserveBank::Instance()->bank;
	ufoText->Draw( x, y,	"ticks=%d/%d Reserve Au=%d G=%d R=%d B=%d V=%d", 
							chitBag->NumTicked(), chitBag->NumChits(),
							w.gold, w.crystal[0], w.crystal[1], w.crystal[2], w.crystal[3] ); 
	y += 16;

	int typeCount[NUM_PLANT_TYPES];
	for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
		typeCount[i] = 0;
		for( int j=0; j<MAX_PLANT_STAGES; ++j ) {
			typeCount[i] += worldMap->plantCount[i][j];
		}
	}
	int stageCount[MAX_PLANT_STAGES];
	for( int i=0; i<MAX_PLANT_STAGES; ++i ) {
		stageCount[i] = 0;
		for( int j=0; j<NUM_PLANT_TYPES; ++j ) {
			stageCount[i] += worldMap->plantCount[j][i];
		}
	}

	ufoText->Draw( x, y,	"Plants type: %d %d %d %d %d %d %d %d stage: %d %d %d %d AIs: norm=%d greater=%d", 
									typeCount[0], typeCount[1], typeCount[2], typeCount[3], typeCount[4], typeCount[5], typeCount[6], typeCount[7],
									stageCount[0], stageCount[1], stageCount[2], stageCount[3],
									chitBag->census.normalMOBs, chitBag->census.greaterMOBs );
	y += 16;

	micropather::CacheData cacheData;
	Vector2I sector = { (int)at.x/SECTOR_SIZE, (int)at.z/SECTOR_SIZE };
	sim->GetWorldMap()->PatherCacheHitMiss( sector, &cacheData );
	ufoText->Draw( x, y, "Pather(%d,%d) kb=%d/%d %.2f cache h:m=%d:%d %.2f", 
						  sector.x, sector.y,
						  cacheData.nBytesUsed/1024,
						  cacheData.nBytesAllocated/1024,
						  cacheData.memoryFraction,
						  cacheData.hit,
						  cacheData.miss,
						  cacheData.hitFraction );
	y += 16;

	Chit* info = sim->GetChitBag()->GetChit( infoID );
	if ( info ) {
		GLString str;
		info->DebugStr( &str );
		ufoText->Draw( x, y, "id=%d: %s", infoID, str.c_str() );
		y += 16;
	}
	if ( !voxelInfoID.IsZero() ) {
		const WorldGrid& wg = sim->GetWorldMap()->GetWorldGrid( voxelInfoID.x, voxelInfoID.y );
		ufoText->Draw( x, y, "voxel=%d,%d hp=%d/%d pave=%d circuit=%d circRot=%d", 
					   voxelInfoID.x, voxelInfoID.y, wg.HP(), wg.TotalHP(), wg.Pave(), wg.Circuit(), wg.CircuitRot() );
		y += 16;
	}
}


void GameScene::SceneResult( int sceneID, int result, const SceneData* data )
{
	if ( sceneID == LumosGame::SCENE_CHARACTER ) {
		// Hardpoint setting is now automatic
	}
	else if ( sceneID == LumosGame::SCENE_MAP ) {
		// Works like a tap. Reconstruct map coordinates.
		Vector2I sector = ((MapSceneData*)data)->destSector;
		if ( !sector.IsZero() ) {
			Vector2F dest = { float(sector.x*SECTOR_SIZE + SECTOR_SIZE/2), float(sector.y*SECTOR_SIZE + SECTOR_SIZE/2) };
			DoDestTapped( dest );
		}
	}
}


