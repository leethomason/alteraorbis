#include "navtest2scene.h"

#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../game/mapspatialcomponent.h"
#include "../game/gamelimits.h"
#include "../game/pathmovecomponent.h"
#include "../game/debugpathcomponent.h"

#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/cameracomponent.h"

#include "../engine/engine.h"
#include "../engine/text.h"
#include "../engine/loosequadtree.h"

#include <ctime>

using namespace grinliz;
using namespace gamui;

//#define DEBUG_PMC

//				doTick (debug, profile)
// 1000 units:	50ms
// using spatial cache:		48ms avoiding 13ms
// impoved spatial cache:	40-43ms avoiding 7ms
//									update 13ms
//  wider bounds, Update massive reduction
//							34ms	update 3ms
//
// 2000 units

NavTest2Scene::NavTest2Scene( LumosGame* game, const NavTest2SceneData* _data ) : Scene( game )
{
	debugRay.direction.Zero();
	debugRay.origin.Zero();

	nChits = 0;
	creationTick = 0;
	game->InitStd( &gamui2D, &okay, 0 );
	engine = 0;
	map = 0;
	data = _data;

	LoadMap();

	RenderAtom atom;
	minimap.Init( &gamui2D, atom, false );
	minimap.SetSize( 200, 200 );

	LayoutCalculator layout = game->DefaultLayout();
	regionButton.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	regionButton.SetSize( layout.Width(), layout.Height() );
	regionButton.SetText( "region" );
}


NavTest2Scene::~NavTest2Scene()
{
	chitBag.DeleteAll();
	delete engine;
	delete map;
}


void NavTest2Scene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	minimap.SetPos( port.UIWidth()-200, 0 );
	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &regionButton, 1, -1 );
}


void NavTest2Scene::LoadMap()
{
	delete engine;
	delete map;

	map = new WorldMap( 32, 32 );
	grinliz::CDynArray<Vector2I> blocks, features;
	map->InitPNG( data->worldFilename, &blocks, &waypoints, &features );

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );	


	for ( int i=0; i<blocks.Size(); ++i ) {
		Chit* chit = chitBag.NewChit();
		const Vector2I& v = blocks[i];
		MapSpatialComponent* msc = new MapSpatialComponent( 1, 1, map );
		chit->Add( msc );
		chit->Add( new RenderComponent( engine, "unitCube", 0 ));

		GET_COMPONENT( chit, MapSpatialComponent )->SetMapPosition( v.x, v.y, 0 );
	}
	for( int i=0; i<features.Size(); ++i ) {
		Chit* chit = chitBag.NewChit();
		const Vector2I& v = features[i];
		MapSpatialComponent* msc = new MapSpatialComponent( 1, 1, map );
		chit->Add( msc );
		chit->Add( new RenderComponent( engine, "tree", 0 ));

		GET_COMPONENT( chit, MapSpatialComponent )->SetMapPosition( v.x, v.y, 0 );
	}

	clock_t start = clock();
	//Performance::ClearSamples();
	for( int i=0; i<waypoints.Size(); ++i ) {
		CreateChit( waypoints[i] );
	}
	//Performance::SetSampling( false );
	clock_t end = clock();
	GLOUTPUT(( "Create chit startup: %d msec\n", (end - start) ));
	engine->CameraLookAt( (float)waypoints[0].x, (float)waypoints[0].y, 40 );
}


void NavTest2Scene::CreateChit( const Vector2I& p )
{
	//GRINLIZ_PERFTRACK;

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent( true ) );

	const char* asset = "humanFemale";
	if ( random.Rand( 4 ) == 0 ) {
		asset = "hornet";
	}

	chit->Add( new RenderComponent( engine, asset, 0 ));
	chit->Add( new PathMoveComponent( map ));
#ifdef DEBUG_PMC
	chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
#endif

	chit->GetSpatialComponent()->SetPosition( (float)p.x+0.5f, 0, (float)p.y+0.5f );
	chit->AddListener( this );
	OnChitMsg( chit, PATHMOVE_MSG_DESTINATION_REACHED, 0 );
	++nChits;
}


void NavTest2Scene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();

	float ratio;
	map->PatherCacheHitMiss( 0, 0, &ratio );

	ufoText->Draw( 0, 16, "PathCache=%.3f hit%%=%d walkers=%d [%d %d %d %d %d]", 
		map->PatherCache(), 
		(int)(ratio*100.f),
		nChits,
		SpaceTree::nModelsAtDepth[0], 
		SpaceTree::nModelsAtDepth[1], 
		SpaceTree::nModelsAtDepth[2], 
		SpaceTree::nModelsAtDepth[3], 
		SpaceTree::nModelsAtDepth[4] );	

	if ( debugRay.direction.x ) {
		Model* root = engine->IntersectModel( debugRay, TEST_TRI, 0, 0, 0, 0 );
		int y = 32;
		for ( ; root; root=root->next ) {
			Chit* chit = root->userData;
			if ( chit ) {
				GLString str;
				chit->DebugStr( &str );
				ufoText->Draw( 0, y, "%s", str.c_str() );
				y += 16;
			}
		}
	}
}


void NavTest2Scene::OnChitMsg( Chit* chit, int id, const ChitEvent* )
{
	if ( id == PATHMOVE_MSG_DESTINATION_REACHED || id == PATHMOVE_MSG_DESTINATION_BLOCKED ) {
		// Reached or blocked, move to next thing:
		const Vector2I& dest = waypoints[random.Rand(waypoints.Size())];
		Vector2F d = { (float)dest.x+0.5f, (float)dest.y+0.5f };
		//GLOUTPUT(( "OnChitMsg %x dest=%.1f,%.1f\n", chit, d.x, d.y ));
		GET_COMPONENT( chit, PathMoveComponent )->QueueDest(d); 
	}
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

	if ( action == GAME_TAP_UP && !uiHasTap ) {
		// Check mini-map
		grinliz::Vector2F ui;
		game->GetScreenport().ViewToUI( view, &ui );
		if (    ui.x >= minimap.X() && ui.x <= minimap.X()+minimap.Width()
			 && ui.y >= minimap.Y() && ui.y <= minimap.Y()+minimap.Height() )
		{
			float x = (ui.x - minimap.X())*(float)map->Width() / minimap.Width();
			float y = (ui.y - minimap.Y())*(float)map->Height() / minimap.Height();

			Vector3F at;
			Rectangle2I mapBounds = map->Bounds();

			//GLOUTPUT(( "Mini-map %d,%d\n", int(x), int(y) ));
			Chit* chit = chitBag.NewChit();
			CameraComponent* cc = new CameraComponent( &engine->camera );
			chit->Add( cc );
			cc->SetAutoDelete( true );

			engine->CameraLookingAt( &at );
			if ( at.x >= 0 && at.z >= 0 && at.x < (float)mapBounds.max.x && at.z < (float)mapBounds.max.y ) {
				Vector3F delta = { x-at.x, 0, y-at.z };
				cc->SetPanTo( engine->camera.PosWC() + delta );
			}
			else {
				Vector3F dest = { x, engine->camera.PosWC().y, y };
				cc->SetPanTo( dest );
			}
		}
	}

	if ( !uiHasTap ) {
		int tap = Process3DTap( action, view, world, engine );
	}
}


void NavTest2Scene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &regionButton ) {
		map->ShowRegionOverlay( regionButton.Down() );
	}
}


void NavTest2Scene::DoTick( U32 deltaTime )
{
	chitBag.DoTick( deltaTime );
	++creationTick;
	
	if ( creationTick >= 5 && nChits < data->nChits ) {
		for( int i=0; i<data->nPerCreation; ++i ) {
			CreateChit( waypoints[random.Rand(waypoints.Size()) ] );
		}
		creationTick = 0;
	}
	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
					 (const void*)engine->GetMiniMapTexture(), 
					 0, 0, 1, 1 );
	minimap.SetAtom( atom );
}


void NavTest2Scene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}

