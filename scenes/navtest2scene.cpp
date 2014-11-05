/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

	creationTick = 0;
	game->InitStd( &gamui2D, &okay, 0 );
	data = _data;

	context.worldMap = new WorldMap( 64, 64 );
	context.engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), context.worldMap);	

	context.chitBag = new LumosChitBag( context, 0 );
	context.worldMap->AttachEngine( 0, context.chitBag );	// connect up the pather, but we do the render.
	context.engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	LoadMap();

	LayoutCalculator layout = game->DefaultLayout();
	regionButton.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	regionButton.SetText( "region" );
}


NavTest2Scene::~NavTest2Scene()
{
	context.chitBag->DeleteAll();
	delete context.chitBag;
	delete context.engine;
	delete context.worldMap;
}


void NavTest2Scene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &regionButton, 1, -1 );
}


void NavTest2Scene::LoadMap()
{
	grinliz::CDynArray<Vector2I> blocks, features;
	context.worldMap->InitPNG( data->worldFilename, &blocks, &waypoints, &features );

	for ( int i=0; i<blocks.Size(); ++i ) {
		Chit* chit = context.chitBag->NewChit();
		const Vector2I& v = blocks[i];
		MapSpatialComponent* msc = new MapSpatialComponent();
		msc->SetMapPosition( v.x, v.y, 1, 1 );
		msc->SetMode( GRID_BLOCKED );
		chit->Add( msc );
		chit->Add( new RenderComponent( "unitCube" ));
	}

	for( int i=0; i<features.Size(); ++i ) {
		const Vector2I& v = features[i];
		context.worldMap->SetPlant(v.x, v.y, 4, 3);
	}

	clock_t start = clock();
	//Performance::ClearSamples();
	for( int i=0; i<waypoints.Size(); ++i ) {
		CreateChit( waypoints[i] );
	}
	//Performance::SetSampling( false );
	clock_t end = clock();
	GLOUTPUT(( "Create chit startup: %d msec\n", (end - start) ));
	context.engine->CameraLookAt( (float)waypoints[0].x, (float)waypoints[0].y, 40 );
}


void NavTest2Scene::CreateChit( const Vector2I& p )
{
	//GRINLIZ_PERFTRACK;

	Chit* chit = context.chitBag->NewChit();
	chit->Add( new SpatialComponent() );

	const char* asset = "humanFemale";
	if ( random.Rand( 4 ) == 0 ) {
		asset = "hornet";
	}

	chit->Add( new RenderComponent( asset ));
	chit->Add( new PathMoveComponent());
#ifdef DEBUG_PMC
	chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
#endif

	chit->GetSpatialComponent()->SetPosition( (float)p.x+0.5f, 0, (float)p.y+0.5f );
	chits.Push(chit->ID());
}


void NavTest2Scene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();
	micropather::CacheData cacheData;

	Vector2I sector = { 0, 0 };
	context.worldMap->PatherCacheHitMiss( sector, &cacheData );

	ufoText->Draw( 0, 16, "PathCache mem%%=%d hit%%=%d walkers=%d", 
		(int)(cacheData.memoryFraction * 100.0f),
		(int)(cacheData.hitFraction * 100.f),
		chits.Size() );	

	if ( debugRay.direction.x ) {
		Model* root = context.engine->IntersectModel( debugRay.origin, debugRay.direction, FLT_MAX, TEST_TRI, 0, 0, 0, 0 );
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


void NavTest2Scene::DoCheck()
{
	for (int i = 0; i < chits.Size(); ++i) {
		Chit* chit = context.chitBag->GetChit(chits[i]);
		if (chit) {
			PathMoveComponent* pmc = GET_SUB_COMPONENT(chit, MoveComponent, PathMoveComponent);
			if (pmc && pmc->Stopped()) {
				// Reached or blocked, move to next thing:
				const Vector2I& dest = waypoints[random.Rand(waypoints.Size())];
				Vector2F d = { (float)dest.x + 0.5f, (float)dest.y + 0.5f };
				//GLOUTPUT(( "OnChitMsg %x dest=%.1f,%.1f\n", chit, d.x, d.y ));
				pmc->QueueDest(d);
			}
		}
		else {
			chits.SwapRemove(i);
			--i;
		}
	}
}


void NavTest2Scene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		context.engine->SetZoom( context.engine->GetZoom() *( 1.0f+delta) );
	else
		context.engine->SetZoom( context.engine->GetZoom() + delta );
}


void NavTest2Scene::Rotate( float degrees ) 
{
	context.engine->camera.Orbit( degrees );
}


void NavTest2Scene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, context.engine);
}


void NavTest2Scene::Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world)
{
	Process3DTap(action, view, world, context.engine);
}


void NavTest2Scene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );

	if ( action == GAME_TAP_UP && !uiHasTap ) {
		// Check mini-map
		grinliz::Vector2F ui;
		game->GetScreenport().ViewToUI( view, &ui );
	}
}


void NavTest2Scene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &regionButton ) {
		Rectangle2I b;
		if ( regionButton.Down() )
			b.Set( 0, 0, context.worldMap->Width()-1, context.worldMap->Height()-1 );
		else
			b.Set( 0, 0, 0, 0 );
		context.worldMap->ShowRegionOverlay( b );
	}
}


void NavTest2Scene::DoTick( U32 deltaTime )
{
	context.chitBag->DoTick( deltaTime );
	++creationTick;
	DoCheck();
	
	if ( creationTick >= 5 && chits.Size() < data->nChits ) {
		for( int i=0; i<data->nPerCreation; ++i ) {
			CreateChit( waypoints[random.Rand(waypoints.Size()) ] );
		}
		creationTick = 0;
	}
}


void NavTest2Scene::Draw3D( U32 deltaTime )
{
	context.engine->Draw( deltaTime );
}

