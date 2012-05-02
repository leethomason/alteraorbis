#include "navtest2scene.h"
#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../engine/engine.h"


using namespace grinliz;
using namespace gamui;


NavTest2Scene::NavTest2Scene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	map = new WorldMap( 32, 32 );
	map->InitCircle();

	engine->SetMap( map );
	engine->CameraLookAt( 10, 10, 40 );

	/*
	for( int i=0; i<NUM_CHITS; ++i ) {
		chit[i] = chitBag.CreateChit();
		chit[i]->Add( new SpatialComponent() );
		chit[i]->Add( new RenderComponent( engine, "humanFemale", MODEL_USER_AVOIDS ) );
		chit[i]->Add( new PathMoveComponent( map, engine->GetSpaceTree() ) );
		chit[i]->Add( new DebugPathComponent( engine, map, game ) );
		chit[i]->GetSpatialComponent()->SetPosition( 10.0f + (float)i*2.f, 0.0f, 10.0f );
	}
	chit[0]->AddListener( this );
	*/
}


NavTest2Scene::~NavTest2Scene()
{
	chitBag.DeleteAll();
	engine->SetMap( 0 );
	delete map;
	delete engine;
}


void NavTest2Scene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );
}


void NavTest2Scene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void NavTest2Scene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void NavTest2Scene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		int tap = Process3DTap( action, view, world, engine );
	}
}


void NavTest2Scene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void NavTest2Scene::DoTick( U32 deltaTime )
{
	chitBag.DoTick( deltaTime );
}


void NavTest2Scene::OnChitMsg( Chit* chit, const char* componentName, int id )
{
	GLOUTPUT(( "OnChitMsg %s %d\n", componentName, id ));
}


void NavTest2Scene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
