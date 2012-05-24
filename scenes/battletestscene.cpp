#include "battletestscene.h"

#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../game/mapspatialcomponent.h"
#include "../game/gamelimits.h"
#include "../game/pathmovecomponent.h"
#include "../game/debugpathcomponent.h"

#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"

#include "../engine/engine.h"
#include "../engine/text.h"

#include <ctime>

using namespace grinliz;
using namespace gamui;


BattleTestScene::BattleTestScene( LumosGame* game ) : Scene( game )
{
	debugRay.direction.Zero();
	debugRay.origin.Zero();

	game->InitStd( &gamui2D, &okay, 0 );
	engine = 0;
	map = 0;

	LoadMap();

//	LayoutCalculator layout = game->DefaultLayout();
}


BattleTestScene::~BattleTestScene()
{
	chitBag.DeleteAll();
	delete engine;
	delete map;
}


void BattleTestScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );

	//const Screenport& port = lumosGame->GetScreenport();
	//LayoutCalculator layout = lumosGame->DefaultLayout();
}


void BattleTestScene::LoadMap()
{
	delete engine;
	delete map;

	map = new WorldMap( 32, 32 );
	grinliz::CDynArray<Vector2I> blocks, features;
	map->InitPNG( "./res/testarena32.png", &blocks, &waypoints, &features );

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

	for( int i=0; i<waypoints.Size(); ++i ) {
		CreateChit( waypoints[i] );
	}
	engine->CameraLookAt( (float)map->Width()/2, (float)map->Height()/2, 45 );
}


void BattleTestScene::CreateChit( const Vector2I& p )
{
	//GRINLIZ_PERFTRACK;

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent( true ) );

	const char* asset = "humanFemale";
	Vector3F trigger = { 0, 0, 0 };

	chit->Add( new RenderComponent( engine, asset, 0 ));
	chit->Add( new PathMoveComponent( map ));

	chit->GetRenderComponent()->GetMetaData( "trigger", &trigger );
	
#ifdef DEBUG_PMC
	chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
#endif


	Chit* weapon = chitBag.NewChit();
	weapon->Add( new RelativeSpatialComponent( false ) );
	weapon->Add( new RenderComponent( engine, "ASLT-1", Model::MODEL_NO_SHADOW ));
	chit->AddListener( weapon->GetSpatialComponent() );

	weapon->GetSpatialComponent()->ToRelative()->SetRelativePosYRot( trigger, 0 );
	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );
}


void BattleTestScene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();

	float ratio;
	map->PatherCacheHitMiss( 0, 0, &ratio );

	ufoText->Draw( 0, 16, "PathCache=%.3f hit%%=%d", 
		map->PatherCache(), 
		(int)(ratio*100.f) );

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


void BattleTestScene::OnChitMsg( Chit* chit, int id )
{
}


void BattleTestScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void BattleTestScene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void BattleTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		int tap = Process3DTap( action, view, world, engine );
	}
}


void BattleTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void BattleTestScene::DoTick( U32 deltaTime )
{
	chitBag.DoTick( deltaTime );
}


void BattleTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}

