#include "gamescene.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"

#include "../game/lumosgame.h"
#include "../game/sim.h"

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

	GLString xmlPath = game->GamePath( "map", 0, "xml" );
	GLString pngPath = game->GamePath( "map", 0, "png" );
	sim->Load( pngPath.c_str(), xmlPath.c_str() );

	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( MINI_MAP_SIZE, MINI_MAP_SIZE );

	atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );
}


GameScene::~GameScene()
{
	delete sim;
}


void GameScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	minimap.SetPos( port.UIWidth()-MINI_MAP_SIZE, 0 );
}



void GameScene::Zoom( int style, float delta )
{
/*	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
*/
}

void GameScene::Rotate( float degrees )
{
//	engine->camera.Orbit( degrees );
}


void GameScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
//	if ( !uiHasTap ) {
//		bool tap = Process3DTap( action, view, world, engine );
//	}
}


void GameScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
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
}

