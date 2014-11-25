
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
#include "../game/adviser.h"
#include "../game/fluidsim.h"

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

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 1.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	dragBuildingRotation = false;
	targetChit = 0;
	possibleChit = 0;
	infoID = 0;
	selectionModel = 0;
	buildActive = 0;
	chitTracking = 0;
	endTimer = 0;
	coreWarningTimer = 0;
	domainWarningTimer = 0;
	poolView = 0;
	attached.Zero();
	voxelInfoID.Zero();
	lumosGame = game;
	adviser = new Adviser();
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

	const RenderAtom nullAtom;
	helpText.Init(&gamui2D);
	helpImage.Init(&gamui2D, nullAtom, true);

	static const char* modeButtonText[NUM_BUILD_MODES] = {
		"Utility", "Denizen", "Agronomy", "Economy", "Visitor", "Circuits"
	};
	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		modeButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		modeButton[i].SetText( modeButtonText[i] );
		modeButton[0].AddToToggleGroup( &modeButton[i] );
	}
	for( int i=0; i<BuildScript::NUM_PLAYER_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );

		buildButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		buildButton[i].SetText( bd.label.safe_str() );

		if (bd.zone == BuildData::ZONE_INDUSTRIAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("anvil", true), game->CalcUIIconAtom("anvil", false));
		else if (bd.zone == BuildData::ZONE_NATURAL)
			buildButton[i].SetDeco(game->CalcUIIconAtom("leaf", true), game->CalcUIIconAtom("leaf", false));

		buildButton[0].AddToToggleGroup( &buildButton[i] );
		modeButton[bd.group].AddSubItem( &buildButton[i] );
	}
	buildButton[0].SetText("UNUSED");

	tabBar0.Init( &gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );
	tabBar1.Init( &gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );

	createWorkerButton.Init( &gamui2D, game->GetButtonLook(0) );
	createWorkerButton.SetText( "WorkerBot" );

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

	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		pickupButton[i].Init( &gamui2D, game->GetButtonLook(0) );
	}

	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	
	RenderAtom grey  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	dateLabel.Init( &gamui2D );
	techLabel.Init( &gamui2D );
	moneyWidget.Init( &gamui2D );
	newsConsole.Init( &gamui2D, sim->GetChitBag() );

	RenderAtom darkPurple = LumosGame::CalcPaletteAtom( 10, 5 );
	darkPurple.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	uiBackground.Init(&gamui2D, darkPurple, false);

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	startGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);
	endGameWidget.Init(&gamui2D, game->GetButtonLook(0), layout);

	Vector3F delta = { 14.0f, 14.0f, 14.0f };
	Vector3F target = { (float)sim->GetWorldMap()->Width() *0.5f, 0.0f, (float)sim->GetWorldMap()->Height() * 0.5f };
	uiMode[UI_VIEW].SetDown();
	if (GetPlayerChit()) {
		target = GetPlayerChit()->GetSpatialComponent()->GetPosition();
	}
	else if (GetHomeCore()) {
		target = GetHomeCore()->ParentChit()->GetSpatialComponent()->GetPosition();
	}
	sim->GetEngine()->CameraLookAt(target + delta, target);
	sim->GetChitBag()->AddListener(this);

	for (int i = 0; i < NUM_BUILD_MARKS; ++i) {
		buildMark[i].Init(&sim->GetWorldMap()->overlay1, grey, false);
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

	for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
		static const char* NAMES[NUM_SQUAD_BUTTONS] = { "Local", "Alpha", "Beta", "Delta", "Omega" };
		squadButton[i].Init(&gamui2D, game->GetButtonLook(0));
		squadButton[i].SetText(NAMES[i]);
		squadButton[0].AddToToggleGroup(&squadButton[i]);
	}
	for (int i = 0; i < MAX_CITIZENS; ++i) {
		squadBar[i].Init(&gamui2D);
	}
}


GameScene::~GameScene()
{
	if ( selectionModel ) {
		sim->GetEngine()->FreeModel( selectionModel );
	}
	delete sim;
	delete adviser;
}


void GameScene::Resize()
{
	const Screenport& port = lumosGame->GetScreenport();
	
	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();

	layout.PosAbs(&censusButton, 0, -1);
	layout.PosAbs(&helpImage, 1, -1);
	layout.PosAbs(&helpText, 2, -1);
	layout.PosAbs(&saveButton, 1, -1);
	layout.PosAbs(&allRockButton, 3, -1);
	layout.PosAbs(&okay, 4, -1);

	layout.PosAbs(&useBuildingButton, 0, 2);

	layout.PosAbs(&cameraHomeButton, 0, 1);
	layout.PosAbs(&prevUnit, 1, 1);
	layout.PosAbs(&avatarUnit, 2, 1);
	layout.PosAbs(&nextUnit, 3, 1);

	for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
		layout.PosAbs(&squadButton[i], i, 1);
	}

	static float SIZE_BOOST = 1.3f;
	helpImage.SetPos(helpImage.X() + layout.Width() * 0.3f, helpImage.Y() - helpImage.Height()*(SIZE_BOOST-1.0f)*0.5f);
	helpImage.SetSize(helpImage.Height()*SIZE_BOOST, helpImage.Height()*SIZE_BOOST);

	int level = BuildScript::GROUP_UTILITY;
	int start = 0;

	for( int i=1; i<BuildScript::NUM_PLAYER_OPTIONS; ++i ) {
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
	techLabel.SetPos( moneyWidget.X() + moneyWidget.Width() + layout.SpacingX(),
					  moneyWidget.Y() );
	layout.PosAbs(&swapWeapons, -1, 5);

	static int CONSOLE_HEIGHT = 2;	// in layout...
	layout.PosAbs(&newsConsole.consoleWidget, 0, -1 - CONSOLE_HEIGHT - 1);
	float consoleHeight = okay.Y() - newsConsole.consoleWidget.Y();
	newsConsole.consoleWidget.SetBounds(0, consoleHeight);

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
	okay.SetVisible(visible);

	// ------- SQUAD LAYOUT ------ //
	layout = static_cast<LumosGame*>(game)->DefaultLayout();
	layout.SetSize(layout.Width(), layout.Height()*0.5f);
	for (int j = 0; j < CITIZEN_BASE; ++j) {
		layout.PosAbs(&squadBar[j], 0, 2*2 + j);
	}
	for (int i = 0; i < MAX_SQUADS; ++i) {
		for (int j = 0; j < SQUAD_SIZE; ++j) {
			layout.PosAbs(&squadBar[CITIZEN_BASE + SQUAD_SIZE*i + j], i+1, 2*2 + j);
		}
	}

	// --- calculated ----
	uiBackground.SetPos(squadBar[0].X(), squadBar[0].Y());
	uiBackground.SetSize(squadBar[MAX_CITIZENS - 1].X() + squadBar[MAX_CITIZENS - 1].Width() - squadBar[0].X(),
						 squadBar[CITIZEN_BASE - 1].Y() + squadBar[CITIZEN_BASE - 1].Height() - squadBar[0].Y());
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
	ModelVoxel mv = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, 0, 0, &at );
	MoveModel( mv.model ? mv.model->userData : 0 );

	if ( mv.model && mv.model->userData ) {
		infoID = mv.model->userData->ID();
	}
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
	Chit* player = GetPlayerChit();
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

	CoreScript* coreScript = GetHomeCore();

	Vector3F atModel = { 0, 0, 0 };
	Vector3F plane = { 0, 0, 0 };
	ModelVoxel mv = ModelAtMouse(view, sim->GetEngine(), TEST_HIT_AABB, 0, MODEL_CLICK_THROUGH, 0, &plane);
	Vector2I plane2i = { (int)plane.x, (int)plane.z };
	if (!map->Bounds().Contains(plane2i)) return;

	const BuildData& buildData = buildScript.GetData(buildActive);
	BuildAction(plane2i);

	if (coreScript && uiMode[UI_CONTROL].Down()) {
		for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
			if (squadButton[i].Down()) {
				ControlTap(i-1, plane2i);
			}
		}
#if 0
		CArray<int, 32> citizens;
		for (int i = 0; i < coreScript->NumCitizens(); ++i) {
			Chit* c = coreScript->CitizenAtIndex(i);
			GLASSERT(c);
			citizens.Push(c->ID());
		}
		coreScript->SetWaypoints(citizens.Mem(), citizens.Size(), plane2i);
#endif

		return;
	}

	if ((game->GetTapMod() & GAME_TAP_MOD_SHIFT) && mv.Hit()) {
		if (AvatarSelected()) {
			// clicked on a rock. Melt away!
			Chit* player = GetPlayerChit();
			if (player && player->GetAIComponent()) {
				//player->GetAIComponent()->RockBreak(mv.Voxel2());
				if (mv.ModelHit())
					player->GetAIComponent()->Target(mv.model->userData, false);
				else
					player->GetAIComponent()->Target(ToWorld2I(mv.voxel), false);
				return;
			}
		}
	}

	Chit* playerChit = GetPlayerChit();
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


bool GameScene::DragRotate(const grinliz::Vector2I& pos2i)
{
	Chit* building = 0;
	if (uiMode[UI_BUILD].Down() && !buildActive) {
		building = sim->GetChitBag()->QueryBuilding(IString(),pos2i,0);
	}
	return building != 0;
}


void GameScene::DragRotateBuilding(const grinliz::Vector2F& drag)
{
	Vector2I dragStart = ToWorld2I(mapDragStart);
	Vector2I dragEnd = ToWorld2I(drag);

	Chit* building = sim->GetChitBag()->QueryBuilding(IString(),dragStart,0);

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
		MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
		if (msc) {
			msc->SetYRotation(rotation);
		}
	}
}


void GameScene::BuildAction(const Vector2I& pos2i)
{
	CoreScript* coreScript = GetHomeCore();
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
			int circuit = sim->GetWorldMap()->Circuit(pos2i.x, pos2i.y);

			if (chit) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				if (msc ) { //&& msc->Building()) {
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
		Vector2I coreSector = cs->ParentChit()->GetSpatialComponent()->GetSector();
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
			dragBuildingRotation = false;
			if (DragAtom(&atom)) {
				mapDragStart = ToWorld2F(at);
			}
			else if (DragRotate(ToWorld2I(at))) {
				dragBuildingRotation = true;
				mapDragStart = ToWorld2F(at);
			}
		}
		if (action == GAME_TAP_MOVE) {
			if (!mapDragStart.IsZero()) {
				Vector2F end = { at.x, at.z };

				if (dragBuildingRotation) {
					// Building rotation.
					DragRotateBuilding(end);
				}
				else {
					// Building placement, clear, etc.
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
		}

		if (action == GAME_TAP_UP) {
			if (mapDragStart.IsZero()) {
				Tap3D(view, world);
			}
			else if (!dragBuildingRotation) {
				Vector2F end = { at.x, at.z };
				Rectangle2I r;
				r.FromPair(ToWorld2I(mapDragStart), ToWorld2I(end));
				DragAtom(&atom);

				for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
					BuildAction(it.Pos());
				}
			}
			dragBuildingRotation = false;
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
		for (int i = 1; i < BuildScript::NUM_PLAYER_OPTIONS; ++i) {
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
	Chit* playerChit = GetPlayerChit();
	CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
	if (playerChit && cc && (playerChit->ID() == cc->Tracking())) {
		return true;
	}
	return false;
}


bool GameScene::AvatarSelected()
{
	bool button = uiMode[UI_VIEW].Down();
	Chit* playerChit = GetPlayerChit();
	if (button && playerChit && playerChit->ID() == chitTracking) {
		return true;
	}
	return false;
}


void GameScene::DoCameraHome()
{
	CoreScript* coreScript = GetHomeCore();
	if (coreScript) {
		Chit* chit = coreScript->ParentChit();
		if (chit && chit->GetSpatialComponent()) {
			Vector3F lookAt = chit->GetSpatialComponent()->GetPosition();
			sim->GetEngine()->CameraLookAt(lookAt.x, lookAt.z);
			CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
			cc->SetTrack(0);
		}
	}
}


bool GameScene::DoEscape()
{
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
	return uiMode[UI_VIEW].Down();
}


void GameScene::DoAvatarButton()
{
	CoreScript* coreScript = GetHomeCore();

	if (AvatarSelected() && CameraTrackingAvatar()) {
		// Teleport.
		if (coreScript && GetPlayerChit()) {
			GetPlayerChit()->GetSpatialComponent()->Teleport(coreScript->ParentChit()->GetSpatialComponent()->GetPosition());
		}
	}
	else {
		// Select.
		chitTracking = GetPlayerChitID();
		Chit* chit = sim->GetChitBag()->GetChit(chitTracking);
		CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
		if (cc && chit) {
			chitTracking = chit->ID();
			cc->SetTrack(chitTracking);
		}
	}
	bool escape = false;
	while (!escape) {
		escape = DoEscape();
	}
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
		MapSceneData* data = new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), GetPlayerChit() );
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
	else if ( item == &allRockButton ) {
		sim->SetAllRock();
	}
	else if ( item == &censusButton ) {
		game->PushScene( LumosGame::SCENE_CENSUS, new CensusSceneData( sim->GetChitBag()) );
	}
	else if ( item == &createWorkerButton ) {
		CoreScript* cs = GetHomeCore();
		if (cs) {
			Chit* coreChit = cs->ParentChit();
			if (coreChit->GetItem()->wallet.Gold() >= WORKER_BOT_COST) {
				ReserveBank::GetWallet()->Deposit(coreChit->GetWallet(), WORKER_BOT_COST);
				int team = coreChit->GetItem()->Team();
				sim->GetChitBag()->NewWorkerChit(coreChit->GetSpatialComponent()->GetPosition(), team);
			}
		}
	}
	else if (item == &abandonButton) {
		// Add a rebuild, if not present, and then abandon.
		CoreScript* cs = GetHomeCore();
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
			chit = GetPlayerChit();
		}
		if ( chit && chit->GetItemComponent() ) {			
			game->PushScene( LumosGame::SCENE_CHARACTER, 
							 new CharacterSceneData( chit->GetItemComponent(), 0, 
							 chit == GetPlayerChit() ? CharacterSceneData::AVATAR : CharacterSceneData::CHARACTER_ITEM, 
							 0, 0 ));
		}
	}
	else if (item == &swapWeapons) {
		GLASSERT(0);
//		Chit* player = GetPlayerChit();
//		if (player && player->GetItemComponent()) {
//			player->GetItemComponent()->SwapWeapons();
//		}
	}
	else if ( item == &useBuildingButton ) {
		sim->UseBuilding();
	}
	else if ( item == &cameraHomeButton ) {
		DoCameraHome();
	}
	else if ( item == &avatarUnit ) {
		DoAvatarButton();
	}
	else if ( item == &prevUnit || item == &nextUnit ) {
		CoreScript* coreScript = GetHomeCore();

		int bias = 0;
		if (item == &prevUnit) bias = -1;
		if (item == &nextUnit) bias = 1;

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
	if (newsConsole.consoleWidget.IsItem(item, &pos2)) {
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
		for (int i = 1; i < BuildScript::NUM_PLAYER_OPTIONS; ++i) {
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
				if (cc && ne.FirstChitID()) {
					cc->SetTrack(ne.FirstChitID());
				}
				else {
					sim->GetEngine()->CameraLookAt(ne.Pos().x, ne.Pos().y);
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
			Chit* playerChit = GetPlayerChit();
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

	Chit* chit = GetPlayerChit();
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
				sectorPort.port   = sim->GetWorldMap()->GetSectorData( sectorPort.sector ).NearestPort( pos );
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
	if (mask == GAME_HK_ESCAPE) {
		DoEscape();
	}
	else if (mask == GAME_HK_CAMERA_AVATAR) {
		DoAvatarButton();
	}
	else if (mask == GAME_HK_CAMERA_CORE) {
		DoCameraHome();
	}
	else if (mask == GAME_HK_SPACE) {
		Chit* playerChit = GetPlayerChit();
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
				Vector2I sector = cs->ParentChit()->GetSpatialComponent()->GetSector();
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
				Vector2I sector = cs->ParentChit()->GetSpatialComponent()->GetSector();
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
#if 1	// Create Human domain.
		{
			CoreScript* cs = CoreScript::GetCore(ToSector(ToWorld2F(at)));
			if (cs) {
				Vector2I sector = cs->ParentChit()->GetSpatialComponent()->GetSector();
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
				Vector2F lookat = truulga->GetSpatialComponent()->GetPosition2D();
				sim->GetEngine()->CameraLookAt(lookat.x, lookat.y);
			}
		}
#endif
#if 0	// Monster swarm
		for (int i = 0; i<5; ++i) {
			sim->GetChitBag()->NewDenizen(ToWorld2I(at), TEAM_GOB);
//			sim->GetChitBag()->NewMonsterChit(at, "mantis", TEAM_GREEN_MANTIS);
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
			Vector2I sector = { (int)at.x / SECTOR_SIZE, (int)at.z / SECTOR_SIZE };
			r.Set( sector.x*SECTOR_SIZE, sector.y*SECTOR_SIZE, (sector.x+1)*SECTOR_SIZE-1, (sector.y+1)*SECTOR_SIZE-1 );
		}
		sim->GetWorldMap()->ShowRegionOverlay( r );
	}
	else if ( mask == GAME_HK_MAP ) {
		Chit* playerChit = GetPlayerChit();
		game->PushScene( LumosGame::SCENE_MAP, new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), playerChit ));
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


/*
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
*/

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
	int maxTubes = CoreScript::MaxCitizens(TEAM_HOUSE, nTemples);

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
	Chit* player = GetPlayerChit();
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


void GameScene::DoTick( U32 delta )
{
	sim->DoTick( delta );

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

	Vector3F lookAt = { 0, 0, 0 };
	sim->GetEngine()->CameraLookingAt( &lookAt );
	Vector2I viewingSector = { (int)lookAt.x / SECTOR_SIZE, (int)lookAt.z / SECTOR_SIZE };
	const SectorData& sd = sim->GetWorldMap()->GetWorldInfo().GetSector( viewingSector );

	CoreScript* coreScript = GetHomeCore();

	// Set the states: VIEW, BUILD, AVATAR. Avatar is 
	// disabled if there isn't one...
	Chit* playerChit = GetPlayerChit();
	if ( !playerChit && !coreScript ) {
		uiMode[UI_VIEW].SetDown();
	}
	uiMode[UI_BUILD].SetEnabled(coreScript != 0);

	Chit* track = sim->GetChitBag()->GetChit( chitTracking );
	if (!track && GetPlayerChit()) {
		track = GetPlayerChit();
	}
	faceWidget.SetFace( &uiRenderer, track ? track->GetItem() : 0 );

	if (playerChit && playerChit->GetAIComponent() && playerChit->GetAIComponent()->GetTarget()) {
		Chit* target = playerChit->GetAIComponent()->GetTarget();
		GLASSERT(target->GetItem());
		targetFaceWidget.SetFace(&uiRenderer, target->GetItem() );
		targetFaceWidget.SetMeta(target->GetItemComponent(), target->GetAIComponent());
	}
	else {
		targetFaceWidget.SetFace(&uiRenderer, 0);
	}

	SetBars(track, track == playerChit);
	CStr<64> str;

	if (coreScript) {
		double sum[ai::Needs::NUM_NEEDS+1] = { 0 };
		bool critical[ai::Needs::NUM_NEEDS+1] = { 0 };
		int nActive = 0;
		Vector2I sector = coreScript->ParentChit()->GetSpatialComponent()->GetSector();

		CChitArray citizens;
		coreScript->Citizens(&citizens);
	
		if (citizens.Size()) {
			for (int i = 0; i < citizens.Size(); i++) {
				Chit* chit = citizens[i];
				if (chit && chit != playerChit && chit->GetSpatialComponent()->GetSector() == sector && chit->GetAIComponent()) {
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
		RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	
		RenderAtom red   = LumosGame::CalcPaletteAtom( 0, 1 );	
		summaryBars.barArr[0]->SetAtom(0, critical[ai::Needs::NUM_NEEDS] ? red : blue);
		summaryBars.barArr[0]->SetRange( float(sum[ai::Needs::NUM_NEEDS]));
		for (int k = 0; k < ai::Needs::NUM_NEEDS; ++k) {
			summaryBars.barArr[k+1]->SetAtom(0, critical[k] ? red : blue);
			summaryBars.barArr[k+1]->SetRange(float(sum[k]));
		}

		int arr[BuildScript::NUM_PLAYER_OPTIONS] = { 0 };
		sim->GetChitBag()->BuildingCounts(sector, arr, BuildScript::NUM_PLAYER_OPTIONS);

		str.Format("Date %.2f\n%s\nPop %d/%d", 
				   sim->AgeF(), 
				   sd.name.safe_str(), 
				   citizens.Size(), CoreScript::MaxCitizens(TEAM_HOUSE, arr[BuildScript::TEMPLE]));
		dateLabel.SetText( str.c_str() );
	}

	// This doesn't really work. The AI will swap weapons
	// at will, so it's more frustrating than useful.
	//swapWeapons.SetVisible(track && track == playerChit);
	swapWeapons.SetVisible(false);
	
	str.Clear();

	Wallet wallet;
	CoreScript* cs = GetHomeCore();
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

	Vector2I homeSector = GetHomeSector();
	{
		// Enforce the worker limit.
		CStr<32> str2;
		static const int MAX_BOTS = 4;

		Rectangle2F b = ToWorld( InnerSectorBounds(homeSector));
		CChitArray arr;
		ItemNameFilter workerFilter(ISC::worker, IChitAccept::MOB);
		sim->GetChitBag()->QuerySpatialHash( &arr, b, 0, &workerFilter );
		nWorkers = arr.Size();

		str2.Format("WorkerBot\n%d %d/%d", WORKER_BOT_COST, nWorkers, MAX_BOTS);		// FIXME: pull price from data
		createWorkerButton.SetText( str2.c_str() );
		createWorkerButton.SetEnabled( arr.Size() < MAX_BOTS );
	}

	{
		LumosChitBag* cb = sim->GetChitBag();
		Vector2I sector = GetHomeSector();
		int arr[BuildScript::NUM_PLAYER_OPTIONS] = { 0 };
		cb->BuildingCounts(sector, arr, BuildScript::NUM_PLAYER_OPTIONS);
		SetBuildButtons(arr);
		adviser->DoTick(delta, coreScript, nWorkers, arr, BuildScript::NUM_PLAYER_OPTIONS);
	}

	abandonButton.SetEnabled(coreScript != 0);
	newsConsole.DoTick( delta, sim->GetChitBag()->GetHomeCore() );

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

	bool squadVisible = uiMode[UI_CONTROL].Down();
	for (int i = 0; i < NUM_SQUAD_BUTTONS; ++i) {
		squadButton[i].SetVisible(squadVisible);
		squadBar[i].SetVisible(squadVisible);
	}
	bool inUse[MAX_CITIZENS] = { false };
	uiBackground.SetVisible(squadVisible);
	if (squadVisible && cs) {
		int count[NUM_SQUAD_BUTTONS] = { 0 };
		GLString str;
		CChitArray citizens;
		cs->Citizens(&citizens);

		for (int i = 0; i < citizens.Size(); ++i) {
			int c = cs->SquadID(citizens[i]->ID()) + 1;
			GLASSERT(c >= 0 && c < NUM_SQUAD_BUTTONS);
			if (count[c] < ((c==0) ? CITIZEN_BASE : SQUAD_SIZE)) {
				const GameItem* item = citizens[i]->GetItem(); 
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
				squadBar[index].Set(item, itemComponent ? itemComponent->GetShield() : 0, 0);
				squadBar[index].SetVisible(true);
				inUse[index] = true;
			}
		}
	}
	for (int i = 0; i < MAX_CITIZENS; ++i) {
		if (!inUse[i]) {
			squadBar[i].SetVisible(false);
		}
	}
}



void GameScene::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	if (msg.ID() == ChitMsg::CHIT_DESTROYED_START) {
		if (chit->GetComponent("CoreScript")) {
			if (sim->GetChitBag()->GetHomeTeam() && (chit->Team() == sim->GetChitBag()->GetHomeTeam())) {
				CoreScript* cs = (CoreScript*) chit->GetComponent("CoreScript");
				GLASSERT(cs);
				Vector2I sector = ToSector(cs->ParentChit()->GetSpatialComponent()->GetPosition2DI());
				const SectorData& sd = sim->GetWorldMap()->GetSectorData(sector);
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
			const SectorData* sd = &sim->GetWorldMap()->GetSectorData(it.Pos());
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
		CoreScript::CreateCore(ToSector(sd->x, sd->y), team, sim->Context());
		ForceHerd(ToSector(sd->x, sd->y));

		ReserveBank* bank = ReserveBank::Instance();
		if (bank) {
			CoreScript* cs = CoreScript::GetCore(ToSector(sd->x, sd->y));
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
	Chit* chit = GetPlayerChit();
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

	const Wallet& w = ReserveBank::Instance()->wallet;
	ufoText->Draw( x, y,	"ticks=%d/%d Reserve Au=%d G=%d R=%d B=%d V=%d", 
							chitBag->NumTicked(), chitBag->NumChits(),
							w.Gold(), w.Crystal(0), w.Crystal(1), w.Crystal(2), w.Crystal(3) ); 
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

	int lesser = 0, greater = 0, denizen = 0;
	chitBag->census.NumByType(&lesser, &greater, &denizen);

	ufoText->Draw( x, y,	"Plants type: %d %d %d %d %d %d %d %d stage: %d %d %d %d AIs: lesser=%d greater=%d denizen=%d", 
									typeCount[0], typeCount[1], typeCount[2], typeCount[3], typeCount[4], typeCount[5], typeCount[6], typeCount[7],
									stageCount[0], stageCount[1], stageCount[2], stageCount[3],
									lesser, greater, denizen );
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
		const MapSceneData* msd = (const MapSceneData*)data;
		// Works like a tap. Reconstruct map coordinates.
		Vector2I sector = msd->destSector;
		if ( !sector.IsZero() ) {
			Vector2F dest = { float(sector.x*SECTOR_SIZE + SECTOR_SIZE/2), float(sector.y*SECTOR_SIZE + SECTOR_SIZE/2) };
			if (msd->view) {
				sim->GetEngine()->CameraLookAt(dest.x, dest.y);
				CameraComponent* cc = sim->GetChitBag()->GetCamera(sim->GetEngine());
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
		return sim->GetPlayerChit();
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
			return cs->ParentChit()->GetSpatialComponent()->GetSector();
		}
	}
	Vector2I z = { 0, 0 };
	return z;
}

