#include "navtestscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

NavTestScene::NavTestScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	map = new WorldMap( 20, 20 );
	map->InitCircle();

	engine->SetMap( map );
	engine->CameraLookAt( 10, 10, 40 );
}


NavTestScene::~NavTestScene()
{
	engine->SetMap( 0 );
	delete map;
	delete engine;
}


void NavTestScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );
}


void NavTestScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void NavTestScene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void NavTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		Ray ray;
		
		switch( action )
		{
			case GAME_TAP_DOWN:
			{
				game->GetScreenport().ViewProjectionInverse3D( &dragMVPI );
				engine->RayFromViewToYPlane( view, dragMVPI, &ray, &dragStart3D );
				dragStartCameraWC = engine->camera.PosWC();
				dragEnd3D = dragStart3D;
				break;
			}

			case GAME_TAP_MOVE:
			case GAME_TAP_UP:
			{
				Vector3F drag;
				engine->RayFromViewToYPlane( view, dragMVPI, &ray, &drag );

				Vector3F delta = drag - dragStart3D;
				delta.y = 0;
				drag.y = 0;
				dragEnd3D = drag;

				engine->camera.SetPosWC( dragStartCameraWC - delta );
				break;
			}
		}
	}
}


void NavTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void NavTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
