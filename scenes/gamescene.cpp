#include "gamescene.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/cameracomponent.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../game/sim.h"
#include "../game/pathmovecomponent.h"
#include "../game/worldmap.h"

#include "../engine/engine.h"
#include "../engine/text.h"

#include "../script/procedural.h"

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 1.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	lumosGame = game;
	game->InitStd( &gamui2D, &okay, 0 );
	sim = new Sim( lumosGame );

	Load();

	Vector3F delta = { 14.0f, 14.0f, 14.0f };
	GLASSERT( sim->GetPlayerChit() );
	Vector3F target = sim->GetPlayerChit()->GetSpatialComponent()->GetPosition();
	sim->GetEngine()->CameraLookAt( target + delta, target );
	
	if ( !sim->GetChitBag()->ActiveCamera() ) {
		Chit* camChit = sim->GetChitBag()->NewChit();
		CameraComponent* cameraComp = new CameraComponent( &sim->GetEngine()->camera );
		camChit->Add( cameraComp );
		cameraComp->SetTrack( sim->GetPlayerChit()->ID() );
	}
	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );
	minimap.SetCapturesTap( true );

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );

	static const char* serialText[NUM_SERIAL_BUTTONS] = { "Save", "Load", "Cycle" }; 
	for( int i=0; i<NUM_SERIAL_BUTTONS; ++i ) {
		serialButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		serialButton[i].SetText( serialText[i] );
	}
	static const char* camModeText[NUM_CAM_MODES] = { "Track", "Teleport" };
	for( int i=0; i<NUM_CAM_MODES; ++i ) {
		camModeButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		camModeButton[i].SetText( camModeText[i] );
		camModeButton[0].AddToToggleGroup( &camModeButton[i] );
	}
	allRockButton.Init( &gamui2D, game->GetButtonLook(0) );
	allRockButton.SetText( "All Rock" );

	//nextPool.Init( &gamui2D, game->GetButtonLook(0) );
	//nextPool.SetText( "Pool" );
	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		newsButton[i].Init( &gamui2D, game->GetButtonLook(0) );
		newsButton[i].SetSize( NEWS_BUTTON_WIDTH, NEWS_BUTTON_HEIGHT );
		newsButton[i].SetText( "news" );
	}
}


GameScene::~GameScene()
{
	delete sim;
}


void GameScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
	
	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	for( int i=0; i<NUM_SERIAL_BUTTONS; ++i ) {
		layout.PosAbs( &serialButton[i], i, -2 );
	}
	for( int i=0; i<NUM_CAM_MODES; ++i ) {
		layout.PosAbs( &camModeButton[i], i, 1 );
	}
	layout.PosAbs( &allRockButton, 0, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	minimap.SetPos( port.UIWidth()-MINI_MAP_SIZE, 0 );

	for( int i=0; i<NUM_NEWS_BUTTONS; ++i ) {
		newsButton[i].SetPos( port.UIWidth()- (NEWS_BUTTON_WIDTH), MINI_MAP_SIZE + (NEWS_BUTTON_HEIGHT+2)*i );
	}
}


void GameScene::Save()
{
	const char* xmlPath = game->GamePath( "map", 0, "xml" );
	const char* datPath = game->GamePath( "map", 0, "dat" );
	const char* gamePath = game->GamePath( "game", 0, "xml" );
	sim->Save( datPath, xmlPath, gamePath );
}


void GameScene::Load()
{
	const char* xmlPath  = lumosGame->GamePath( "map", 0, "xml" );
	const char* pngPath  = lumosGame->GamePath( "map", 0, "png" );
	const char* datPath  = lumosGame->GamePath( "map", 0, "dat" );
	const char* gamePath = lumosGame->GamePath( "game", 0, "xml" );
	if ( lumosGame->HasFile( gamePath )) {
		sim->Load( datPath, xmlPath, gamePath );
	}
	else {
		sim->Load( datPath, xmlPath, 0 );
	}
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


void GameScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
	Engine* engine = sim->GetEngine();

	if ( !uiHasTap ) {
		bool tap = Process3DTap( action, view, world, sim->GetEngine() );

		if ( tap ) {
			Matrix4 mvpi;
			Ray ray;
			Vector3F at;
			game->GetScreenport().ViewProjectionInverse3D( &mvpi );
			sim->GetEngine()->RayFromViewToYPlane( view, mvpi, &ray, &at );

			int tapMod = lumosGame->GetTapMod();

			if ( tapMod == 0 ) {
				Chit* chit = sim->GetPlayerChit();
				if ( chit ) {
					if ( camModeButton[TRACK].Down() ) {
						PathMoveComponent* pmc = GET_COMPONENT( chit, PathMoveComponent );
						if ( pmc ) {
							Vector2F dest = { at.x, at.z };
							pmc->QueueDest( dest );
						}
					}
					else if ( camModeButton[TELEPORT].Down() ) {
						SpatialComponent* sc = chit->GetSpatialComponent();
						GLASSERT( sc );
						sc->SetPosition( at.x, 0, at.z );
					}
				}
			}
			else if ( tapMod == GAME_TAP_MOD_CTRL ) {
				sim->CreateVolcano( (int)at.x, (int)at.z );
			}
		}
	}
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &minimap ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );
		GLOUTPUT(( "minimap tapped nx=%.1f ny=%.1f\n", x, y ));

		Engine* engine = sim->GetEngine();
		Vector2F dest = { 0, 0 };
		dest.x = x*(float)engine->GetMap()->Width();
		dest.y = y*(float)engine->GetMap()->Height();

		Chit* chit = sim->GetPlayerChit();
		if ( chit ) {
			if ( camModeButton[TRACK].Down() ) {
				PathMoveComponent* pmc = GET_COMPONENT( chit, PathMoveComponent );
				if ( pmc ) {
					pmc->QueueDest( dest );
				}
			}
			else if ( camModeButton[TELEPORT].Down() ) {
				SpatialComponent* sc = chit->GetSpatialComponent();
				GLASSERT( sc );
				sc->SetPosition( dest.x, 0, dest.y );
			}
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
	else if ( item == &serialButton[CYCLE] ) {
		Save();
		delete sim;
		sim = new Sim( lumosGame );
		Load();
	}
	else if ( item == &allRockButton ) {
		sim->SetAllRock();
	}
	/*
	else if ( item == &nextPool ) {
		Chit* chit = sim->GetPlayerChit();
		if ( chit ) {
			SpatialComponent* sc = chit->GetSpatialComponent();
			GLASSERT( sc );

			const Vector3F& pos = sc->GetPosition();
			int zx = (int)(pos.x) / WorldMap::ZONE_SIZE;
			int zy = (int)(pos.z) / WorldMap::ZONE_SIZE;
			int zindex = zy * WorldMap::DZONE + zx;

			const WorldMap::ZoneInfo* zi = sim->GetWorldMap()->GetZoneInfo();
			int search = zindex+1;
			if ( search == WorldMap::DZONE2 ) search = 0;

			while ( true ) {
				if ( zi[search].pools ) {
					sc->SetPosition( (float)(zi[search].x+WorldMap::ZONE_SIZE/2),
									 0,
									 (float)(zi[search].y+WorldMap::ZONE_SIZE/2) );
					break;
				}
				++search;
				if ( search == zindex ) break;
				if ( search == WorldMap::DZONE2 )
					search = 0;
			}
		}
	}
	*/
}


void GameScene::HandleHotKey( int mask )
{
	super::HandleHotKey( mask );
}


void GameScene::DoTick( U32 delta )
{
	sim->DoTick( delta );
}


void GameScene::Draw3D( U32 deltaTime )
{
	sim->Draw3D( deltaTime );

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					 (const void*)sim->GetMiniMapTexture(), 
					 0, 0, 1, 1 );
	/* coordinate test. they are correct.
	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					 (const void*)TextureManager::Instance()->GetTexture( "palette" ),
					 0, 0, 1, 1 );
	*/

	minimap.SetAtom( atom );

	Chit* chit = sim->GetPlayerChit();
	if ( chit && chit->GetSpatialComponent() ) {
		const Vector3F& v = chit->GetSpatialComponent()->GetPosition();
		Map* map = sim->GetEngine()->GetMap();
		
		float x = minimap.X() + Lerp( 0.f, minimap.Width(),  v.x / (float)map->Width() );
		float y = minimap.Y() + Lerp( 0.f, minimap.Height(), v.z / (float)map->Height() );

		playerMark.SetCenterPos( x, y );
	}
}


void GameScene::DrawDebugText()
{
	DrawDebugTextDrawCalls( 16, sim->GetEngine() );

	UFOText* ufoText = UFOText::Instance();
	Chit* chit = sim->GetPlayerChit();
	Engine* engine = sim->GetEngine();

	if ( chit && chit->GetSpatialComponent() ) {
		const Vector3F& v = chit->GetSpatialComponent()->GetPosition();
		ufoText->Draw( 0, 32, "Player: %.1f, %.1f, %.1f  Camera: %.1f %.1f %.1f", 
			           v.x, v.y, v.z,
					   engine->camera.PosWC().x, engine->camera.PosWC().y, engine->camera.PosWC().z );
	}
	ufoText->Draw( 0, 48, "Tap world or map to go to location. End/Home rotate, PgUp/Down zoom." );
}

