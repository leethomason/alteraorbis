#include "battletestscene.h"

#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../game/mapspatialcomponent.h"
#include "../game/gamelimits.h"
#include "../game/pathmovecomponent.h"
#include "../game/debugpathcomponent.h"
#include "../game/debugstatecomponent.h"
#include "../game/gameitem.h"
#include "../game/aicomponent.h"
#include "../game/healthcomponent.h"

#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/inventorycomponent.h"
#include "../xegame/itemcomponent.h"

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
	engine->particleSystem->LoadParticleDefs( "./res/particles.xml" );

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

	/*
	for( int i=0; i<waypoints.Size(); ++i ) {
		CreateChit( waypoints[i] );
	}
	*/
	Vector2I unit = { 2, 16 };
	Vector2I dummy = { 16, 16 };
	CreateChit( unit );
	CreateChit( dummy );
	dummy.Set( 16, 17 );
	CreateChit( dummy );

	engine->CameraLookAt( (float)map->Width()/2, (float)map->Height()/2, 
		                  42.f,		// height
						  225.f );	// rotattion

	// Trigger the AI to do something.
	ChitEvent event( AI_EVENT_AWARENESS );
	event.bounds.Set( 0, 0, (float)(map->Width()), (float)(map->Height()) );

	event.data0 = 0;	
	chitBag.QueueEvent( event );
	event.data0 = 1;	
	chitBag.QueueEvent( event );
}


void BattleTestScene::CreateChit( const Vector2I& p )
{
	//GRINLIZ_PERFTRACK;

	enum {
		HUMAN=1,
		HORNET,
		DUMMY
	};

	int team = HUMAN;
	const char* asset = "humanFemale";
	if ( p.x > 27 ) {
		team = HORNET;
		asset = "hornet";
	}
	else if ( p.x > 6 && p.x < 27 ) {
		team = DUMMY;
		asset = "prime";
	}

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent( true ) );
	chit->Add( new RenderComponent( engine, asset, 0 ));
	chit->Add( new PathMoveComponent( map ));
	if ( team == HUMAN || team == HORNET ) {
		chit->Add( new AIComponent( engine, map ));
	}
	GameItem* item = new GameItem();
	item->primaryTeam = team;
	chit->Add( new ItemComponent( item ));

	//chit->Add( new DebugStateComponent( map ));
	chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
	chit->Add( new HealthComponent());
	InventoryComponent* inv = new InventoryComponent( &chitBag );
	chit->Add( inv );

	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );
	GET_COMPONENT( chit, HealthComponent )->SetHealth( 100, 100 );

	if ( team == HUMAN || team == HORNET ) {
		WeaponItem* gunItem = new WeaponItem( "ASLT-1", "ASLT-1" );

		Chit* gun = chitBag.NewChit();
		gun->Add( new RenderComponent( engine, "ASLT-1", Model::MODEL_NO_SHADOW ));
		gun->Add( new ItemComponent( gunItem ));

		inv->AddToInventory( gun );
	}
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


void BattleTestScene::OnChitMsg( Chit* chit, int id, const ChitEvent* )
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

