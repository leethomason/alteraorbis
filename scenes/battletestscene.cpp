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
	{ COUNT_1,	"1",		LEFT_COUNT },
	{ COUNT_2,	"2",		LEFT_COUNT },
	{ COUNT_4,	"4",		LEFT_COUNT },
	{ COUNT_8,	"8",		LEFT_COUNT },
	{ DUMMY,	"Dummy",	LEFT_MOB },
	{ HUMAN,	"Human",	LEFT_MOB },
	{ MANTIS,	"Mantis",	LEFT_MOB },
	{ BALROG,	"Balrog",	LEFT_MOB },
	{ NO_WEAPON,"None",		LEFT_WEAPON },
	{ MELEE_WEAPON, "Melee",LEFT_WEAPON },
	{ PISTOL,	"Pistol",	LEFT_WEAPON },

	{ COUNT_1,	"1",		RIGHT_COUNT },
	{ COUNT_2,	"2",		RIGHT_COUNT },
	{ COUNT_4,	"4",		RIGHT_COUNT },
	{ COUNT_8,	"8",		RIGHT_COUNT },
	{ DUMMY,	"Dummy",	RIGHT_MOB },
	{ HUMAN,	"Human",	RIGHT_MOB },
	{ MANTIS,	"Mantis",	RIGHT_MOB },
	{ BALROG,	"Balrog",	RIGHT_MOB },
	{ NO_WEAPON,"None",		RIGHT_WEAPON },
	{ MELEE_WEAPON, "Melee",RIGHT_WEAPON },
	{ PISTOL,	"Pistol",	RIGHT_WEAPON },
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
	itemStorage.Load( "./res/itemdef.xml" );

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


int BattleTestScene::ButtonDownID( int group )
{
	for( int i=0; i<NUM_BUTTONS; ++i ) {
		if (    buttonDef[i].group == group
			 && optionButton[i].Enabled()
			 && optionButton[i].Down() ) 
		{
			return buttonDef[i].id;
		}
	}
	GLASSERT( 0 );
	return -1;
}


void BattleTestScene::LoadMap()
{
	delete engine;
	delete map;

	map = new WorldMap( 32, 32 );
	grinliz::CDynArray<Vector2I> blocks, features, wp;
	map->InitPNG( "./res/testarena32.png", &blocks, &wp, &features );

	for( int i=0; i<wp.Size(); ++i ) {
		Vector2I v = wp[i];
		if ( v.x < map->Width() / 3 ) {
			waypoints[LEFT].Push( v );
		}
		else if ( v.x > map->Width()*2/3 ) {
			waypoints[RIGHT].Push( v );
		}
		else {
			waypoints[MID].Push( v );
		}
	}
	ShuffleArray( waypoints[RIGHT].Mem(), waypoints[RIGHT].Size(), &random );
	ShuffleArray( waypoints[LEFT].Mem(),  waypoints[LEFT].Size(),  &random );
	ShuffleArray( waypoints[MID].Mem(),   waypoints[MID].Size(),   &random );

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
	CreateChit( unit, HUMAN, PISTOL, LEFT );
	CreateChit( dummy, DUMMY, NO_WEAPON, MID );
	dummy.Set( 16, 17 );
	CreateChit( dummy, DUMMY, NO_WEAPON, MID );

	engine->CameraLookAt( (float)map->Width()/2, (float)map->Height()/2, 
		                  22.f,		// height
						  225.f );	// rotation
}


void BattleTestScene::GoScene()
{
	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );

	// Remove everything that is currently on the board that is some sort of character.
	CDynArray<Chit*> arr;
	chitBag.QuerySpatialHash( &arr, b, 0, GameItem::CHARACTER, false );
	for( int i=0; i<arr.Size(); ++i ) {
		chitBag.DeleteChit( arr[i] );
	}

	int leftCount	= 1 << ButtonDownID( LEFT_COUNT );
	int leftMoB		= ButtonDownID( LEFT_MOB );
	int leftWeapon	= ButtonDownID( LEFT_WEAPON );
	int rightCount	= 1 << ButtonDownID( RIGHT_COUNT );
	int rightMoB	= ButtonDownID( RIGHT_MOB );
	int rightWeapon = ButtonDownID( RIGHT_WEAPON );
	int rightLoc    = (rightMoB == DUMMY) ? MID : RIGHT;

	for( int i=0; i<leftCount; ++i ) {
		CreateChit( waypoints[LEFT][i], leftMoB, leftWeapon, LEFT );
	}
	for( int i=0; i<rightCount; ++i ) {
		CreateChit( waypoints[rightLoc][i], rightMoB, rightWeapon, rightLoc );
	}

	// Trigger the AI to do something.
	AwarenessChitEvent event( LEFT, b );
	event.SetItemFilter( GameItem::CHARACTER );
	chitBag.QueueEvent( event );
}

		
void BattleTestScene::CreateChit( const Vector2I& p, int type, int loadout, int team )
{
	const char* asset = "humanFemale";
	switch ( type ) {
		case DUMMY:			asset="dummytarget";	break;
		case MANTIS:		asset="mantis";			break;
		case BALROG:		asset="balrog";			break;
	}

	ItemStorage::GameItemArr itemDefArr;
	itemStorage.Get( asset, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = chitBag.NewChit();
	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, asset, 0 ));
	if ( type != DUMMY ) {
		chit->Add( new PathMoveComponent( map ));
		chit->Add( new AIComponent( engine, map ));
		chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
	}
	else {
		chit->Add( new MoveComponent());
	}

	GameItem item( *(itemDefArr[0]));
	item.primaryTeam = team;
	chit->Add( new ItemComponent( item ));

	chit->Add( new HealthComponent());
	InventoryComponent* inv = new InventoryComponent( &chitBag );
	chit->Add( inv );

	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );

	for( int i=1; i<itemDefArr.Size(); ++i ) {
		GameItem intrinsic( *(itemDefArr[i] ));
		inv->AddToInventory( intrinsic, true );
	}

	if ( type == HUMAN ) {
		if( loadout == MELEE_WEAPON ) {
			const GameItem *knife = itemStorage.Get( "testknife" );
			GLASSERT( knife );
			inv->AddToInventory( *knife, true );
		}
		else if ( loadout == PISTOL ) {
			const GameItem* gun = itemStorage.Get( "testgun" );
			GLASSERT( gun );
			inv->AddToInventory( *gun, true );
		}
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


void BattleTestScene::OnChitMsg( Chit* chit, const ChitMsg& msg )
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

