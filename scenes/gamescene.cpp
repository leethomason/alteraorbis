#include "gamescene.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/cameracomponent.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../game/sim.h"
#include "../game/pathmovecomponent.h"

#include "../engine/engine.h"
#include "../engine/text.h"

#include "../script/procedural.h"

using namespace grinliz;
using namespace gamui;

static const float DEBUG_SCALE = 2.0f;
static const float MINI_MAP_SIZE = 150.0f*DEBUG_SCALE;
static const float MARK_SIZE = 6.0f*DEBUG_SCALE;

GameScene::GameScene( LumosGame* game ) : Scene( game )
{
	lumosGame = game;
	game->InitStd( &gamui2D, &okay, 0 );
	sim = new Sim( lumosGame );

	//game->LoadGame();

	Load();

	Vector3F delta = { 20.0f, 20.0f, 20.0f };
	Vector3F target = sim->GetPlayerChit()->GetSpatialComponent()->GetPosition();
	sim->GetEngine()->CameraLookAt( target + delta, target );

	Chit* camChit = sim->GetChitBag()->NewChit();
	CameraComponent* cameraComp = new CameraComponent( &sim->GetEngine()->camera );
	camChit->Add( cameraComp );
	cameraComp->SetTrack( sim->GetPlayerChit()->ID() );

	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );
	minimap.SetCapturesTap( true );

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );

	refresh.Init( &gamui2D, game->GetButtonLook(0) );
	refresh.SetText( "Refresh" );
}


GameScene::~GameScene()
{
	delete sim;
}


void GameScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
	
	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	layout.PosAbs( &refresh, 0, -2 );

	const Screenport& port = lumosGame->GetScreenport();
	minimap.SetPos( port.UIWidth()-MINI_MAP_SIZE, 0 );
}


void GameScene::Save()
{
	GLString xmlPath = game->GamePath( "map", 0, "xml" );
	GLString pngPath = game->GamePath( "map", 0, "png" );
	GLString gamePath = game->GamePath( "game", 0, "xml" );
	sim->Save( pngPath.c_str(), xmlPath.c_str(), gamePath.c_str() );
}


void GameScene::Load()
{
	GLString xmlPath  = lumosGame->GamePath( "map", 0, "xml" );
	GLString pngPath  = lumosGame->GamePath( "map", 0, "png" );
	GLString gamePath = lumosGame->GamePath( "game", 0, "xml" );
	if ( lumosGame->HasFile( gamePath.c_str() )) {
		sim->Load( pngPath.c_str(), xmlPath.c_str(), gamePath.c_str() );
	}
	else {
		sim->Load( pngPath.c_str(), xmlPath.c_str(), 0 );
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

			Chit* chit = sim->GetPlayerChit();
			if ( chit ) {
				PathMoveComponent* pmc = GET_COMPONENT( chit, PathMoveComponent );
				if ( pmc ) {
					Vector2F dest = { at.x, at.z };
					pmc->QueueDest( dest );
				}
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
			PathMoveComponent* pmc = GET_COMPONENT( chit, PathMoveComponent );
			if ( pmc ) {
				pmc->QueueDest( dest );
			}
		}
	}
	else if ( item == &refresh ) {
		Save();
	}
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
	if ( chit && chit->GetSpatialComponent() ) {
		const Vector3F& v = chit->GetSpatialComponent()->GetPosition();
		ufoText->Draw( 0, 32, "Player: %.1f, %.1f, %.1f", v.x, v.y, v.z );
	}
	ufoText->Draw( 0, 48, "Tap world or map to go to location. End/Home rotate, PgUp/Down zoom." );
}

