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

const BattleTestScene::ButtonDef BattleTestScene::buttonDef[NUM_BUTTONS] =
{
	{ COUNT_1,	"1",		0 },
	{ COUNT_2,	"2",		0 },
	{ COUNT_4,	"4",		0 },
	{ COUNT_8,	"8",		0 },
	{ DUMMY,	"Dummy",	1 },
	{ HUMAN,	"Human",	1 },
	{ MANTIS,	"Mantis",	1 },
	{ BALROG,	"Balrog",	1 },
	{ NO_WEAPON,"None",		2 },
	{ MELEE_WEAPON, "Melee",2 },
	{ PISTOL,	"Pistol",	2 },

	{ COUNT_1,	"1",		10 },
	{ COUNT_2,	"2",		10 },
	{ COUNT_4,	"4",		10 },
	{ COUNT_8,	"8",		10 },
	{ DUMMY,	"Dummy",	11 },
	{ HUMAN,	"Human",	11 },
	{ MANTIS,	"Mantis",	11 },
	{ BALROG,	"Balrog",	11 },
	{ NO_WEAPON,"None",		12 },
	{ MELEE_WEAPON, "Melee",12 },
	{ PISTOL,	"Pistol",	12 },
};


BattleTestScene::BattleTestScene( LumosGame* game ) : Scene( game )
{
	debugRay.direction.Zero();
	debugRay.origin.Zero();

	game->InitStd( &gamui2D, &okay, 0 );
	LayoutCalculator layout = game->DefaultLayout();

	const ButtonLook& look = game->GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	const float width  = layout.Width();
	const float height = layout.Height();

	goButton.Init( &gamui2D, look );
	goButton.SetText( "Go!" );
	goButton.SetSize( width, height );

	int currentGroup = -1;
	ToggleButton* toggle = 0;

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		optionButton[i].Init( &gamui2D, look );
		optionButton[i].SetSize( width, height*0.5f );
		optionButton[i].SetText( buttonDef[i].label );
		if ( currentGroup != buttonDef[i].group ) {
			currentGroup = buttonDef[i].group;
			toggle = &optionButton[i];
			//optionButton[i].SetDown();
		}
		else {
			toggle->AddToToggleGroup( &optionButton[i] );
		}
	}

	optionButton[HUMAN].SetDown();
	optionButton[DUMMY].SetVisible( false );
	optionButton[COUNT_4+NUM_OPTIONS].SetDown();

	// WIP:
	optionButton[MANTIS].SetEnabled( false );	optionButton[MANTIS+NUM_OPTIONS].SetEnabled( false );
	optionButton[BALROG].SetEnabled( false );	optionButton[BALROG+NUM_OPTIONS].SetEnabled( false );
	optionButton[BALROG].SetEnabled( false );	optionButton[BALROG+NUM_OPTIONS].SetEnabled( false );

	engine = 0;
	map = 0;

	LoadMap();
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

	const Screenport& port = lumosGame->GetScreenport();
	LayoutCalculator layout = lumosGame->DefaultLayout();

	layout.PosAbs( &okay, 0, -1 );
	layout.PosAbs( &goButton, 0, -2 );

	layout.SetSize( layout.Width(), layout.Height()*0.5f );

	int currentGroup = -1;
	int y = -1;
	int x = 0;

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		if ( buttonDef[i].group != currentGroup ) {
			y = i== NUM_OPTIONS ? 0 : y+1;
			x = i < NUM_OPTIONS ? 0 : -4;
			currentGroup = buttonDef[i].group;
		}
		layout.PosAbs( &optionButton[i], x, y );
		++x;
	}
}


void BattleTestScene::LoadMap()
{
	delete engine;
	delete map;

	map = new WorldMap( 32, 32 );
	grinliz::CDynArray<Vector2I> blocks, features;
	map->InitPNG( "./res/testarena32.png", &blocks, &waypoints, &features );

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );	
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

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

	Vector2I unit = { 2, 16 };
	Vector2I dummy = { 16, 16 };
	CreateChit( unit, HUMAN, PISTOL );
	CreateChit( dummy, DUMMY, NO_WEAPON );
	dummy.Set( 16, 17 );
	CreateChit( dummy, DUMMY, NO_WEAPON );

	engine->CameraLookAt( (float)map->Width()/2, (float)map->Height()/2, 
		                  22.f,		// height
						  225.f );	// rotattion
}


void BattleTestScene::GoScene()
{
	// Trigger the AI to do something.
	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );
	AwarenessChitEvent event( HUMAN, b );
	chitBag.QueueEvent( event );
}

		
void BattleTestScene::CreateChit( const Vector2I& p, int team, int loadout )
{
	const char* asset = "humanFemale";
	switch ( team ) {
		case DUMMY:			asset="dummytarget";	break;
		case MANTIS:		asset="mantis";			break;
		case BALROG:		asset="balrog";			break;
	}

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent( true ));
	chit->Add( new RenderComponent( engine, asset, 0 ));
	chit->Add( new PathMoveComponent( map ));
	if ( team != DUMMY ) {
		chit->Add( new AIComponent( engine, map ));
	}

	GameItem item( GameItem::CHARACTER );
	item.primaryTeam = team;
	chit->Add( new ItemComponent( item ));

	chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
	chit->Add( new HealthComponent());
	InventoryComponent* inv = new InventoryComponent( &chitBag );
	chit->Add( inv );

	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );
	GET_COMPONENT( chit, HealthComponent )->SetHealth( 100, 100 );

	if ( team == HUMAN ) {
		GameItem hand( GameItem::MELEE_WEAPON | GameItem::APPENDAGE );
		inv->AddToInventory( hand );

		if( loadout == MELEE_WEAPON ) {
			GameItem knife( GameItem::MELEE_WEAPON | GameItem::HELD,
						  "testknife", "testknife", "trigger" );
			inv->AddToInventory( knife );
		}
		else if ( loadout == PISTOL ) {
			GameItem gun( GameItem::MELEE_WEAPON | GameItem::RANGED_WEAPON | GameItem::HELD,
						  "testgun", "testgun", "trigger" );
			inv->AddToInventory( gun );
		}
	}
	else if ( team == BALROG ) {
		// FIXME kinetic damage bonus
		GameItem claw( GameItem::MELEE_WEAPON | GameItem::APPENDAGE );
		inv->AddToInventory( claw );
	}
	else if ( team == MANTIS ) {
		// FIXME kinetic damage bonus
		GameItem pincer( GameItem::MELEE_WEAPON | GameItem::APPENDAGE );
		inv->AddToInventory( pincer );
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
	else if ( item == &goButton ) {
		GoScene();
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

