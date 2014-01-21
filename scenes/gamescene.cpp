#include <time.h>

#include "gamescene.h"
#include "characterscene.h"

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

#include "../scenes/mapscene.h"
#include "../scenes/censusscene.h"

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 1.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

#define USE_MOUSE_MOVE_SELECTION

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
	chitFaceToTrack = 0;
	currentNews = 0;
	voxelInfoID.Zero();
	lumosGame = game;
	game->InitStd( &gamui2D, &okay, 0 );
	sim = new Sim( lumosGame );

	Load();

	Vector3F delta = { 14.0f, 14.0f, 14.0f };
	Vector3F target = { (float)sim->GetWorldMap()->Width() *0.5f, 0.0f, (float)sim->GetWorldMap()->Height() * 0.5f };
	if ( sim->GetPlayerChit() ) {
		target = sim->GetPlayerChit()->GetSpatialComponent()->GetPosition();
	}
	sim->GetEngine()->CameraLookAt( target + delta, target );
	
	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );
	minimap.SetCapturesTap( true );

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );

	static const char* serialText[NUM_SERIAL_BUTTONS] = { "Save", "Load" }; 
	for( int i=0; i<NUM_SERIAL_BUTTONS; ++i ) {
		serialButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		serialButton[i].SetText( serialText[i] );
	}

	useBuildingButton.Init( &gamui2D, game->GetButtonLook(0) );
	useBuildingButton.SetText( "Use" );
	useBuildingButton.SetVisible( false );

	cameraHomeButton.Init( &gamui2D, game->GetButtonLook(0) );
	cameraHomeButton.SetText( "Home" );
	cameraHomeButton.SetVisible( false );

	static const char* modeButtonText[NUM_BUILD_MODES] = {
		"Utility", "Tech0", "Tech1", "Tech2", "Tech3"
	};
	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		modeButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		modeButton[i].SetText( modeButtonText[i] );
		modeButton[0].AddToToggleGroup( &modeButton[i] );
	}
	for( int i=0; i<BuildScript::NUM_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );

		buildButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		buildButton[i].SetText( bd.label.c_str() );
		buildButton[0].AddToToggleGroup( &buildButton[i] );

		modeButton[bd.techLevel].AddSubItem( &buildButton[i] );
	}

	tabBar.Init( &gamui2D, LumosGame::CalcUIIconAtom( "tabBar", true ), false );

	createWorkerButton.Init( &gamui2D, game->GetButtonLook(0) );
	createWorkerButton.SetText( "WorkerBot" );

	for( int i=0; i<NUM_UI_MODES; ++i ) {
		static const char* TEXT[NUM_UI_MODES] = { "Build", "View", "Avatar" };
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
	clearButton.Init( &gamui2D, game->GetButtonLook(0) );
	clearButton.SetSize( NEWS_BUTTON_WIDTH, NEWS_BUTTON_HEIGHT );
	clearButton.SetText( "Clear" );

	faceWidget.Init( &gamui2D, game->GetButtonLook(0), FaceWidget::ALL );

	chitFaceToTrack = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;
	faceWidget.SetSize( 100, 100 );

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

	uiMode[UI_AVATAR].SetDown();
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
	lumosGame->PositionStd( &okay, 0 );
	
	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	for( int i=0; i<NUM_SERIAL_BUTTONS; ++i ) {
		layout.PosAbs( &serialButton[i], i+1, -1 );
	}
	layout.PosAbs( &allRockButton, 1, -2 );
	layout.PosAbs( &censusButton, 2, -2 );
	layout.PosAbs( &useBuildingButton, 1, 0 );
	layout.PosAbs( &cameraHomeButton, 1, 0 );

	int level = BuildScript::TECH_UTILITY;
	int start = 0;

	for( int i=0; i<BuildScript::NUM_OPTIONS; ++i ) {
		const BuildData& bd = buildScript.GetData( i );
		if ( bd.techLevel != level ) {
			level = bd.techLevel;
			start = i;
		}
		layout.PosAbs( &buildButton[i], i-start+1, 1 );
	}
	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		layout.PosAbs( &modeButton[i], i+1, 0 );
	}
	tabBar.SetPos( modeButton[0].X(), modeButton[0].Y() );
	tabBar.SetSize( modeButton[NUM_BUILD_MODES-1].X() + modeButton[NUM_BUILD_MODES-1].Width() - modeButton[0].X(), modeButton[0].Height() );

	layout.PosAbs( &createWorkerButton, 1, 2 );
	for( int i=0; i<NUM_UI_MODES; ++i ) {
		layout.PosAbs( &uiMode[i], 0, i );
	}

	layout.PosAbs( &faceWidget, -1, 0, 1, 1 );
	layout.PosAbs( &minimap,    -2, 0, 1, 1 );
	minimap.SetSize( minimap.Width(), minimap.Width() );	// make square
	faceWidget.SetSize( faceWidget.Width(), faceWidget.Width() );

	layout.PosAbs( &dateLabel,   -3, 0 );
	layout.PosAbs( &moneyWidget, 5, -1 );
	techLabel.SetPos( moneyWidget.X() + moneyWidget.Width() + layout.SpacingX(),
					  moneyWidget.Y() );

	static int CONSOLE_HEIGHT = 2;	// in layout...
	layout.PosAbs( &consoleWidget, 1, -1 - CONSOLE_HEIGHT );
	float consoleHeight = okay.Y() - consoleWidget.Y();
	consoleWidget.SetBounds( 0, consoleHeight );

	for( int i=0; i<NUM_PICKUP_BUTTONS; ++i ) {
		layout.PosAbs( &pickupButton[i], 0, i+3 );
	}

	// ------ CHANGE LAYOUT ------- //
	layout.SetSize( faceWidget.Width(), 20.0f );
	layout.SetSpacing( 5.0f );

	bool visible = game->GetDebugUI();
	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		layout.PosAbs( &newsButton[i], -1, -NUM_NEWS_BUTTONS+i );
		newsButton[i].SetVisible( visible );
	}
	clearButton.SetPos( newsButton[0].X() - clearButton.Width(), newsButton[0].Y() );

	allRockButton.SetVisible( visible );
	clearButton.SetVisible( visible );
	censusButton.SetVisible( visible );
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

	if ( lumosGame->HasFile( gamePath )) {
		sim->Load( datPath, gamePath );
	}
	else {
		sim->Load( datPath, 0 );
	}
	chitFaceToTrack = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;
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
	ModelVoxel mv = this->ModelAtMouse( view, sim->GetEngine(), TEST_TRI, 0, 0, 0, 0 );
	MoveModel( mv.model ? mv.model->userData : 0 );

	if ( mv.model && mv.model->userData ) {
		infoID = mv.model->userData->ID();
	}
	if ( mv.VoxelHit() ) {
		voxelInfoID = mv.Voxel2();
	}

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
		if ( buildActive == BuildScript::ROTATE ) {
			// nothing.
		}
		else if (    buildActive == BuildScript::CLEAR 
			      || buildActive == BuildScript::PAVE ) 
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
		if ( ai && ai->GetTeamStatus( target ) == RELATE_ENEMY ) {
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

	if ( ai && ai->GetTeamStatus( target ) == RELATE_ENEMY ) {
		ai->Target( target, true );
		setTarget = "target";
	}
	else if ( ai && FreeCameraMode() && ai->GetTeamStatus( target ) == RELATE_FRIEND ) {
		const GameItem* item = target->GetItem();
		bool denizen = strstr( item->ResourceName(), "human" ) != 0;
		if (denizen) {
			chitFaceToTrack = target->ID();
			setTarget = "possibleTarget";

			CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
			if ( cc ) {
				cc->SetTrack( chitFaceToTrack );
			}
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



void GameScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
	Engine* engine = sim->GetEngine();
	WorldMap* map = sim->GetWorldMap();

	enable3DDragging = FreeCameraMode();
	
	buildActive = 0;
	if ( uiMode[UI_BUILD].Down() ) {
		for( int i=1; i<BuildScript::NUM_OPTIONS; ++i ) {
			if ( buildButton[i].Down() ) {
				buildActive = i;
				break;
			}
		}
	}
	SetSelectionModel( view );

	if ( !uiHasTap ) {
		Vector3F atModel = { 0, 0, 0 };
		Vector3F plane   = { 0, 0, 0 };
		ModelVoxel mv = ModelAtMouse( view, sim->GetEngine(), TEST_HIT_AABB, 0, MODEL_CLICK_THROUGH, 0, &plane );
		if ( plane.x > 0 && plane.z > 0 && plane.x < float(map->Width()) && plane.z < float(map->Width()) ) {
			// okay
		}
		else {
			return;	// outside of world. don't do testing.
		}
		Vector2I plane2i = { (int)plane.x, (int)plane.z };
		const BuildData& buildData = buildScript.GetData( buildActive );

		bool tap = Process3DTap( action, view, world, sim->GetEngine() );

		if ( tap ) {
			if ( uiMode[UI_BUILD].Down() ) {
				CoreScript* coreScript = sim->GetChitBag()->GetCore( sim->GetChitBag()->GetHomeSector() );
				WorkQueue* wq = coreScript->GetWorkQueue();
				GLASSERT( wq );
				RemovableFilter removableFilter;

				if ( buildActive == BuildScript::CLEAR ) {
#ifdef USE_MOUSE_MOVE_SELECTION
					wq->AddAction( plane2i, BuildScript::CLEAR );
#else
					if ( mv.VoxelHit()) {
						wq->Add( WorkQueue::CLEAR, mv.Voxel2(), IString() );
					}
					else if ( mv.ModelHit() && mv.model->userData && removableFilter.Accept( mv.model->userData )) {
						MapSpatialComponent* msc = GET_SUB_COMPONENT( mv.model->userData, SpatialComponent, MapSpatialComponent );
						GameItem* gameItem = mv.model->userData->GetItem();
						GLASSERT( msc );
						if ( msc && gameItem  ) {
							wq->Add( WorkQueue::CLEAR, msc->MapPosition(), gameItem->name );
						}
					}
#endif
				}
				else if ( buildActive == BuildScript::NONE ) {
#ifdef USE_MOUSE_MOVE_SELECTION
					wq->Remove( plane2i );
#else
					if ( mv.VoxelHit() ) {
						// Clear a voxel.
						wq->Remove( mv.Voxel2() );
					}
					else if ( mv.ModelHit() ) {
						MapSpatialComponent* msc = GET_SUB_COMPONENT( mv.model->userData, SpatialComponent, MapSpatialComponent );
						if ( msc ) {
							wq->Remove( msc->MapPosition() );
						}
					}
#endif
				}
				else if ( buildActive == BuildScript::ROTATE ) {
					if ( mv.ModelHit() ) {
						MapSpatialComponent* msc = GET_SUB_COMPONENT( mv.model->userData, SpatialComponent, MapSpatialComponent );
						if ( msc ) {
							float r = msc->GetYRotation();
							r += 90.0f;
							r = NormalizeAngleDegrees( r );
							msc->SetYRotation( r );
						}
					}
				}
#ifdef USE_MOUSE_MOVE_SELECTION
				else {
#else
				else if ( !mv.VoxelHit() ) {
#endif
					wq->AddAction( plane2i, buildActive );
				}
			}
			
			if ( mv.VoxelHit() ) {
				// clicked on a rock. Melt away!
				Chit* player = sim->GetPlayerChit();
				if ( player && player->GetAIComponent() ) {
					player->GetAIComponent()->RockBreak( mv.Voxel2() );
					return;
				}
			}

			int tapMod = lumosGame->GetTapMod();

			if ( tapMod == 0 ) {
				Chit* playerChit = sim->GetPlayerChit();
				if ( mv.model ) {
					TapModel( mv.model->userData );
				}
				else if ( playerChit ) {
					Vector2F dest = { plane.x, plane.z };
					DoDestTapped( dest );
				}
				else if ( FreeCameraMode() ) {
					sim->GetEngine()->CameraLookAt( plane.x, plane.z );
				}
			}
			else if ( tapMod == GAME_TAP_MOD_CTRL ) {

				Vector2I v = { (int)plane.x, (int)plane.z };
				sim->CreatePlayer( v );
				chitFaceToTrack = sim->GetPlayerChit() ? sim->GetPlayerChit()->ID() : 0;
#if 0
				sim->CreateVolcano( (int)at.x, (int)at.z, 6 );
				sim->CreatePlant( (int)at.x, (int)at.z, -1 );
#endif
			}
			else if ( tapMod == GAME_TAP_MOD_SHIFT ) {
				//for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
				//	sim->CreatePlant( (int)plane.x+i, (int)plane.z, i );
				//}
				Vector3F p = plane;
				p.y = 0;
				for( int i=0; i<5; ++i ) {
					sim->GetChitBag()->NewMonsterChit( plane, "mantis", TEAM_GREEN_MANTIS );
					p.x += 1.0f;
				}
			}
		}
	}
}


bool GameScene::CoreMode()
{
	return uiMode[UI_BUILD].Down() || uiMode[UI_VIEW].Down(); 
}

bool GameScene::FreeCameraMode()
{
	//bool button = freeCameraButton.Down();
	bool button      = uiMode[UI_VIEW].Down();
	Chit* playerChit = sim->GetPlayerChit();
	bool coreMode    = CoreMode();
	
	if ( button || coreMode || (!playerChit) )
		return true;
	return false;
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	Vector2F dest = { 0, 0 };

	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &minimap ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );
		GLOUTPUT(( "minimap tapped nx=%.1f ny=%.1f\n", x, y ));

		Engine* engine = sim->GetEngine();

		if ( FreeCameraMode() ) {
			dest.x = x*(float)engine->GetMap()->Width();
			dest.y = y*(float)engine->GetMap()->Height();
		}
		else {
			Vector2I sector;
			sector.x = int( x * float(NUM_SECTORS));
			sector.y = int( y * float(NUM_SECTORS));

			MapSceneData* data = new MapSceneData( sim->GetChitBag(), sim->GetWorldMap(), sim->GetPlayerChit() );
			data->destSector = sector;
			game->PushScene( LumosGame::SCENE_MAP, data );
		}
	}
	else if ( item == &serialButton[SAVE] ) {
		Save();
	}
	else if ( item == &serialButton[LOAD] ) {
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
		Chit* coreChit = cs->ParentChit();
		if ( coreChit->GetItem()->wallet.gold >= 20 ) {
			coreChit->GetItem()->wallet.gold -= 20;
			ReserveBank::Instance()->Deposit( 20 );
			int team = coreChit->GetItem()->primaryTeam;
			sim->GetChitBag()->NewWorkerChit( coreChit->GetSpatialComponent()->GetPosition(), team );
		}
	}
	else if ( item == faceWidget.GetButton() ) {
		Chit* chit = sim->GetChitBag()->GetChit( chitFaceToTrack );
		if ( chit && chit->GetItemComponent() ) {			
			game->PushScene( LumosGame::SCENE_CHARACTER, 
							 new CharacterSceneData( chit->GetItemComponent(), 0, 0 ));
		}
	}
	else if ( item == &useBuildingButton ) {
		sim->UseBuilding();
	}
	else if ( item == &cameraHomeButton ) {
		CoreScript* coreScript = sim->GetChitBag()->GetCore( sim->GetChitBag()->GetHomeSector() );
		if ( coreScript ) {
			Chit* chit = coreScript->ParentChit();
			if ( chit && chit->GetSpatialComponent() ) {
				Vector3F lookAt = chit->GetSpatialComponent()->GetPosition();
				sim->GetEngine()->CameraLookAt( lookAt.x, lookAt.z );
			}
		}
	}

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		if ( item == &newsButton[i] ) {
			if ( FreeCameraMode() ) {
				NewsHistory* history = NewsHistory::Instance();
				const grinliz::CArray< NewsEvent, NewsHistory::MAX_CURRENT >& current = history->CurrentNews();

				int index = current.Size() - 1 - i;
				if ( index >= 0 ) {
					const NewsEvent& ne = current[index];
					CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
					if ( cc && ne.chitID ) {
						cc->SetTrack( ne.chitID );
					}
					else {
						sim->GetEngine()->CameraLookAt( ne.pos.x, ne.pos.y );
						if ( cc ) cc->SetTrack( 0 );
					}
				}
			}
		}
	}
	if ( item == &clearButton ) {
		if ( FreeCameraMode() ) {
			CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
			if ( cc  ) {
				cc->SetTrack( 0 );
			}
		}
	}
	if ( item == &uiMode[UI_BUILD] || item == &uiMode[UI_VIEW] || item == &uiMode[UI_AVATAR] ) {
		// Set it to track nothing; if it needs to track something, that will
		// be set by future mouse actions or DoTick
		CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
		if ( cc ) {
			cc->SetTrack( 0 );
		}
	}

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

	if ( !FreeCameraMode() ) {
		Chit* chit = sim->GetPlayerChit();
		if ( chit ) {
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
		else {
			sim->GetEngine()->CameraLookAt( dest.x, dest.y );
		}
	}
	else {
		CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
		if ( cc ) {
			cc->SetTrack( 0 );
		}		
		sim->GetEngine()->CameraLookAt( dest.x, dest.y );
	}
}



void GameScene::HandleHotKey( int mask )
{
	if ( mask == GAME_HK_TOGGLE_FAST ) {
		fastMode = !fastMode;
	}
	else if ( mask == GAME_HK_SPACE ) {
		Chit* playerChit = sim->GetPlayerChit();
		if ( playerChit ) {
#if 0
			ItemComponent* ic = playerChit->GetItemComponent();
			if ( ic ) {
				ic->SwapWeapons();
			}
#else
			AIComponent* ai = playerChit->GetAIComponent();
			ai->Rampage( 0 );
#endif
		}
	}
	else if ( mask == GAME_HK_TOGGLE_COLORS ) {
		static int colorSeed = 0;
		static const char* NAMES[4] = { "kiosk.m", "kiosk.n", "kiosk.c", "kiosk.s" };
		ItemNameFilter filter( NAMES, 4 );
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
		Chit* playerChit = sim->GetPlayerChit();
		if ( playerChit ) {
			static const int GOLD = 100;
			ReserveBank::Instance()->WithdrawGold( GOLD );
			playerChit->GetItem()->wallet.AddGold( GOLD );
		}
	}
	else if ( mask == GAME_HK_CHEAT_CRYSTAL ) {
		Chit* playerChit = sim->GetPlayerChit();
		if ( playerChit ) {
			for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
				ReserveBank::Instance()->WithdrawCrystal( i );
				playerChit->GetItem()->wallet.AddCrystal(i);
			}
		}
	}
	else if ( mask == GAME_HK_CHEAT_TECH ) {
		Chit* player = sim->GetPlayerChit();

		CoreScript* coreScript = sim->GetChitBag()->GetCore( sim->GetChitBag()->GetHomeSector() );
		for( int i=0; i<10; ++i ) {
			coreScript->AddTech();
		}
	}
	else {
		super::HandleHotKey( mask );
	}
}


void GameScene::SetPickupButtons()
{
	Chit* player = sim->GetPlayerChit();
	if ( player && uiMode[UI_AVATAR].Down() ) {
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
	NewsHistory* history = NewsHistory::Instance();
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

	for ( ;currentNews < history->NumNews(); ++currentNews ) {
		const NewsEvent& ne = history->News( currentNews );
		//Vector2I sector = ne.Sector();

		str = "";

		switch( ne.what ) {
		case NewsEvent::DENIZEN_CREATED:
		case NewsEvent::DENIZEN_KILLED:
		case NewsEvent::FORGED:
		case NewsEvent::UN_FORGED:
		case NewsEvent::PURCHASED:
			if ( coreScript && coreScript->IsCitizen( ne.chitID )) {
				ne.Console( &str );
			}
			break;

		case NewsEvent::GREATER_MOB_CREATED:
		case NewsEvent::GREATER_MOB_KILLED:
			ne.Console( &str );
			break;

		case NewsEvent::LESSER_MOB_NAMED:
		case NewsEvent::LESSER_NAMED_MOB_KILLED:
			{
				Vector2I sector = ToSector( ToWorld2I( ne.pos ));
				if ( sector == homeSector || sector == avatarSector ) {
					ne.Console( &str );
				}
			}
			break;


		default:
			break;
		}
		if ( !str.empty() ) {
			consoleWidget.Push( str );
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
		NewsHistory* history = NewsHistory::Instance();
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
	Vector2I sector = { (int)lookAt.x / SECTOR_SIZE, (int)lookAt.z / SECTOR_SIZE };
	const SectorData& sd = sim->GetWorldMap()->GetWorldInfo().GetSector( sector );

	CStr<64> str;
	str.Format( "Date %.2f\n%s", NewsHistory::Instance()->AgeF(), sd.name.c_str() );
	dateLabel.SetText( str.c_str() );

	// Set the states: VIEW, BUILD, AVATAR. Avatar is 
	// disabled if there isn't one...
	Chit* playerChit = sim->GetPlayerChit();
	if ( !playerChit ) {
		if ( uiMode[UI_AVATAR].Down() ) {
			uiMode[UI_VIEW].SetDown();
		}
	}
	uiMode[UI_AVATAR].SetEnabled( playerChit != 0 );
	if ( uiMode[UI_AVATAR].Down() ) {
		chitFaceToTrack = playerChit ? playerChit->ID() : 0;
	}

	Chit* track = sim->GetChitBag()->GetChit( chitFaceToTrack );
	faceWidget.SetFace( &uiRenderer, track ? track->GetItem() : 0 );
	SetBars( track );
	
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
//	xpLabel.SetText( str.c_str() );

	for( int i=0; i<NUM_BUILD_MODES; ++i ) {
		modeButton[i].SetVisible( uiMode[UI_BUILD].Down() );
	}
	tabBar.SetVisible( uiMode[UI_BUILD].Down() );
	createWorkerButton.SetVisible( uiMode[UI_BUILD].Down() );

	str.Clear();
	CoreScript* coreScript = sim->GetChitBag()->GetCore( sim->GetChitBag()->GetHomeSector() );

	float tech = coreScript->GetTech();
	int maxTech = coreScript->MaxTech();
	str.Format( "Tech %.2f / %d", tech, maxTech );
	techLabel.SetText( str.c_str() );

	if ( playerChit && CoreMode() ) {

		int atech = coreScript->AchievedTechLevel();
		for( int i=1; i<NUM_BUILD_MODES; ++i ) {
			modeButton[i].SetEnabled( i-1 <= atech );
		}

		CStr<32> str2;
		// How many workers in the sector?
		Vector2I sector = ToSector( playerChit->GetSpatialComponent()->GetPosition2DI() );
		Rectangle2F b;
		b.Set( (float)(sector.x*SECTOR_SIZE+1), (float)(sector.y*SECTOR_SIZE+1),
			   (float)((sector.x+1)*SECTOR_SIZE-1), (float)((sector.y+1)*SECTOR_SIZE-1) );

		CChitArray arr;
		ItemNameFilter workerFilter( IStringConst::worker );

		static const int MAX_BOTS = 4;
		sim->GetChitBag()->QuerySpatialHash( &arr, b, 0, &workerFilter );
		str2.Format( "WorkerBot\n20 %d/%d", arr.Size(), MAX_BOTS ); 
		createWorkerButton.SetText( str2.c_str() );
		createWorkerButton.SetEnabled( arr.Size() < MAX_BOTS );
	}
	consoleWidget.DoTick( delta );
	ProcessNewsToConsole();

	bool useBuildingVisible = false;
	if ( uiMode[UI_AVATAR].Down() && playerChit ) {
		Chit* building = sim->GetChitBag()->QueryPorch( playerChit->GetSpatialComponent()->GetPosition2DI() );
		if ( building ) {
			useBuildingVisible = true;
		}
	}
	useBuildingButton.SetVisible( useBuildingVisible );
	cameraHomeButton.SetVisible( uiMode[UI_VIEW].Down() );


	// It's pretty tricky keeping the camera, camera component, and various
	// modes all working together. 
	// - If we aren't in FreeCam mode, we should be looking at the player.

	if ( !FreeCameraMode() ) {
		if ( playerChit ) {
			CameraComponent* cc = sim->GetChitBag()->GetCamera( sim->GetEngine() );
			cc->SetTrack( playerChit->ID() );
		}
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
	static const int x = 200;
	int y = (int)game->GetScreenport().UIHeight() - 160;
	DrawDebugTextDrawCalls( x, y, sim->GetEngine() );
	y += 16;

	UFOText* ufoText = UFOText::Instance();
	Chit* chit = sim->GetPlayerChit();
	Engine* engine = sim->GetEngine();
	LumosChitBag* chitBag = sim->GetChitBag();
	Vector3F at = { 0, 0, 0 };

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

	Wallet w = ReserveBank::Instance()->GetWallet();
	ufoText->Draw( x, y,	"Date=%.2f %s. Sim/S=%.1f x%.1f ticks=%d/%d Reserve Au=%d r%dg%dv%d", 
							NewsHistory::Instance()->AgeF(),
							fastMode ? "fast" : "norm", 
							simPS,
							simPS / 30.0f,
							chitBag->NumTicked(), chitBag->NumChits(),
							w.gold, w.crystal[0], w.crystal[1], w.crystal[2] ); 
	y += 16;

	int typeCount[NUM_PLANT_TYPES];
	for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
		typeCount[i] = 0;
		for( int j=0; j<MAX_PLANT_STAGES; ++j ) {
			typeCount[i] += chitBag->census.plants[i][j];
		}
	}
	int stageCount[MAX_PLANT_STAGES];
	for( int i=0; i<MAX_PLANT_STAGES; ++i ) {
		stageCount[i] = 0;
		for( int j=0; j<NUM_PLANT_TYPES; ++j ) {
			stageCount[i] += chitBag->census.plants[j][i];
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
		ufoText->Draw( x, y, "voxel=%d,%d hp=%d/%d", voxelInfoID.x, voxelInfoID.y, wg.HP(), wg.TotalHP() );
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


