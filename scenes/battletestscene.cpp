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
#include "../engine/loosequadtree.h"

#include "../script/worldscript.h"

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
	battleStarted = false;

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
	GLASSERT( waypoints[RIGHT].Size() == 8 );
	GLASSERT( waypoints[LEFT].Size() == 8 );
	GLASSERT( waypoints[MID].Size() == 8 );
	ShuffleArray( waypoints[RIGHT].Mem(), waypoints[RIGHT].Size(), &random );
	ShuffleArray( waypoints[LEFT].Mem(),  waypoints[LEFT].Size(),  &random );
	ShuffleArray( waypoints[MID].Mem(),   waypoints[MID].Size(),   &random );

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );	
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	engine->SetGlow( true );
	chitBag.SetEngine( engine );

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



void BattleTestScene::HandleHotKeyMask( int mask )
{
	if ( mask == GAME_HK_TOGGLE_GLOW ) {
		engine->SetGlow( !engine->Glow() );
	}
}

void BattleTestScene::GoScene()
{
	GLLOG(( "---- GO ---- \n" ));
	battleStarted = true;
	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );

	// Remove everything that is currently on the board that is some sort of character.
	chitBag.QuerySpatialHash( &chitArr, b, 0, GameItem::CHARACTER );
	for( int i=0; i<chitArr.Size(); ++i ) {
		chitBag.DeleteChit( chitArr[i] );
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
	for( int i=LEFT; i<=RIGHT; ++i ) {
		AwarenessChitEvent* event = new AwarenessChitEvent( i, b );
		event->SetItemFilter( GameItem::CHARACTER );
		chitBag.QueueEvent( event );
	}
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

	const char* weaponNames[3] = { "none", "melee", "pistol" };
	GLLOG(( "Chit Created: %d '%s' weapon='%s' team=%d\n",
		    chit->ID(), 
			asset, weaponNames[loadout-NO_WEAPON], team ));

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, asset, 0 ));
	if ( type != DUMMY ) {
		chit->Add( new PathMoveComponent( map ));
		chit->Add( new AIComponent( engine, map ));
		//chit->Add( new DebugPathComponent( engine, map, static_cast<LumosGame*>(game) ));
	}
	else {
		// The avoider only avoids things with move components. Makes sense and yet
		// stretches this. (Note: BLOCKS are an entirely different matter. The pather
		// handles those just fine.)
		chit->Add( new MoveComponent());
	}
	chit->Add( new DebugStateComponent( map ));

	GameItem item( *(itemDefArr[0]));
	item.primaryTeam = team;
	chit->Add( new ItemComponent( item ));

	chit->Add( new HealthComponent());
	InventoryComponent* inv = new InventoryComponent( &chitBag );
	chit->Add( inv );

	chit->AddListener( this );
	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );

	for( int i=1; i<itemDefArr.Size(); ++i ) {
		inv->AddToInventory( new GameItem( *(itemDefArr[i]) ), true );
	}

	if ( type == HUMAN ) {
		/* Not working yet.
		// Always get a shield
		{
			const GameItem* shield = itemStorage.Get( "shield" );
			GLASSERT( shield );
			inv->AddToInventory( new GameItem( *shield ), true );
		}
		*/

		if( loadout == MELEE_WEAPON ) {
			const GameItem *knife = itemStorage.Get( "testknife" );
			GLASSERT( knife );
			inv->AddToInventory( new GameItem(*knife), true );
		}
		else if ( loadout == PISTOL ) {
			const GameItem* gun = itemStorage.Get( "testgun" );
			GLASSERT( gun );
			inv->AddToInventory( new GameItem(*gun), true );
		}
	}
	else if ( type == BALROG ) {
		if ( loadout == MELEE_WEAPON ) {
			const GameItem* ax = itemStorage.Get( "ax" );
			GLASSERT( ax );
			inv->AddToInventory( new GameItem(*ax), true );
		}
	}
}


void BattleTestScene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();

	float ratio;
	map->PatherCacheHitMiss( 0, 0, &ratio );

	ufoText->Draw( 0, 16, "PathCache=%.3f hit%%=%d chits:ticked/total=%d/%d", 
		map->PatherCache(), 
		(int)(ratio*100.f),
		chitBag.NumTicked(), chitBag.NumChits() );

	if ( debugRay.direction.x ) {
		Vector3F at;
		Model* root = engine->IntersectModel( debugRay.origin, debugRay.direction, FLT_MAX, TEST_TRI, 0, 0, 0, &at );

		if ( root ) {
			engine->particleSystem->EmitPD( "spell", at, V3F_UP, engine->camera.EyeDir3(), 0 );

			int y = 32;
			Chit* chit = root->userData;
			if ( chit ) {
				GLString str;
				chit->DebugStr( &str );
				ufoText->Draw( 0, y, "%s ", str.c_str() );
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
		bool tap = Process3DTap( action, view, world, engine );
		if ( action == GAME_TAP_DOWN ) {

			Vector3F at;
			IntersectRayPlane( debugRay.origin, debugRay.direction, 1, 0, &at );
			at.y = 0.01f;

			DamageDesc dd;
			dd.Set( 20, 0, 0, 0 );
			BattleMechanics::GenerateExplosionMsgs( dd, at, engine, &chitArr );
		}
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
	/*
	boltTimer += deltaTime;
	if ( boltTimer > 500 ) {
		boltTimer = 0;
		Bolt* bolt = chitBag.NewBolt();

		bolt->dir.Set( -0.1f+fuzz.Uniform()*0.2f, 0, 1 );
		bolt->head.Set( 0.5f * (float)map->Width(), 0.5f, 2 );
		bolt->color.Set( 1, 0.1f, 0.3f, 1 );
		bolt->damage.vec[ DamageDesc::KINETIC ] = 20;
	}
	*/

	chitBag.DoTick( deltaTime, engine );

	if ( battleStarted ) {
		bool aware = false;
		Rectangle2F b;
		b.Set( 0, 0, (float)map->Width(), (float)map->Height() );

		chitBag.QuerySpatialHash( &chitArr, b, 0, GameItem::CHARACTER );
		for( int i=0; i<chitArr.Size(); ++i ) {
			Chit* c = chitArr[i];

			AIComponent* ai = GET_COMPONENT( c, AIComponent );
			if ( ai && ai->AwareOfEnemy() ) {
				aware = true;
				break;
			}
		}
		if ( !aware ) {
			AwarenessChitEvent* event = new AwarenessChitEvent( LEFT, b );
			event->SetItemFilter( GameItem::CHARACTER );
			chitBag.QueueEvent( event );
		}
	}
}


void BattleTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime, chitBag.BoltMem(), chitBag.NumBolts() );
}

