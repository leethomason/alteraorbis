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
#include "../game/layout.h"

#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"

#include "../engine/engine.h"
#include "../engine/text.h"
#include "../engine/loosequadtree.h"
#include "../engine/particle.h"

#include "../script/worldscript.h"
#include "../script/procedural.h"

#include <ctime>

using namespace grinliz;
using namespace gamui;

const BattleTestScene::ButtonDef BattleTestScene::buttonDef[NUM_BUTTONS] =
{
	{ COUNT_1,	"1",		LEFT_COUNT },
	{ COUNT_2,	"2",		LEFT_COUNT },
	{ COUNT_4,	"4",		LEFT_COUNT },
	{ COUNT_8,	"8",		LEFT_COUNT },
	{ DUMMY,	"Dummy",			LEFT_MOB },
	{ HUMAN,	"Human",			LEFT_MOB },
	{ ARACHNOID,"Arachnoid",		LEFT_MOB },
	{ MANTIS,	"Mantis",			LEFT_MOB },
	{ RED_MANTIS,"RedMantis",		LEFT_MOB },
	{ CYCLOPS,	"Cyclops",			LEFT_MOB },
	{ FIRE_CYCLOPS,	"F-Cyclops",	LEFT_MOB },
	{ SHOCK_CYCLOPS,	"S-Cyclops",LEFT_MOB },
	{ TROLL,	"Troll",	LEFT_MOB },
	{ NO_WEAPON,"None",		LEFT_WEAPON },
	{ MELEE_WEAPON, "Melee",LEFT_WEAPON },
	{ PISTOL, "Pistol", LEFT_WEAPON },
	{ BLASTER, "Blaster", LEFT_WEAPON },
	{ PULSE, "Pulse", LEFT_WEAPON },
	{ BEAMGUN, "Beamgun", LEFT_WEAPON },
	{ BLASTER_AND_GUN,	"Both",	LEFT_WEAPON },
	{ LEVEL_0,	"Lev0",		LEFT_LEVEL },
	{ LEVEL_2,	"Lev2",		LEFT_LEVEL },
	{ LEVEL_4,	"Lev4",		LEFT_LEVEL },
	{ LEVEL_8,	"Lev8",		LEFT_LEVEL },

	{ COUNT_1,	"1",		RIGHT_COUNT },
	{ COUNT_2,	"2",		RIGHT_COUNT },
	{ COUNT_4,	"4",		RIGHT_COUNT },
	{ COUNT_8,	"8",		RIGHT_COUNT },
	{ DUMMY,	"Dummy",			RIGHT_MOB },
	{ HUMAN,	"Human",			RIGHT_MOB },
	{ ARACHNOID,"Arachnoid",		RIGHT_MOB },
	{ MANTIS,	"Mantis",			RIGHT_MOB },
	{ RED_MANTIS,	"RedMantis",	RIGHT_MOB },
	{ CYCLOPS,	"Cyclops",			RIGHT_MOB },
	{ FIRE_CYCLOPS,	"F-Cyclops",	RIGHT_MOB },
	{ SHOCK_CYCLOPS,	"S-Cyclops",RIGHT_MOB },
	{ TROLL,	"Troll",	RIGHT_MOB },
	{ NO_WEAPON,"None",		RIGHT_WEAPON },
	{ MELEE_WEAPON, "Melee",RIGHT_WEAPON },
	{ PISTOL, "Pistol", RIGHT_WEAPON },
	{ BLASTER, "Blaster", RIGHT_WEAPON },
	{ PULSE, "Pulse", RIGHT_WEAPON },
	{ BEAMGUN, "Beamgun", RIGHT_WEAPON },
	{ BLASTER_AND_GUN, "Both", RIGHT_WEAPON },
	{ LEVEL_0,	"Lev0",		RIGHT_LEVEL },
	{ LEVEL_2,	"Lev2",		RIGHT_LEVEL },
	{ LEVEL_4,	"Lev4",		RIGHT_LEVEL },
	{ LEVEL_8,	"Lev8",		RIGHT_LEVEL },
};


BattleTestScene::BattleTestScene( LumosGame* game ) : Scene( game )
{
	debugRay.direction.Zero();
	debugRay.origin.Zero();
	battleStarted = false;
	fireTestGun = false;

	game->InitStd( &gamui2D, &okay, 0 );
	LayoutCalculator layout = game->DefaultLayout();

	const ButtonLook& look = game->GetButtonLook( LumosGame::BUTTON_LOOK_STD );

	goButton.Init( &gamui2D, look );
	goButton.SetText( "Go!" );

	regionButton.Init( &gamui2D, look );
	regionButton.SetText( "region" );

	int currentGroup = -1;
	ToggleButton* toggle = 0;

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		optionButton[i].Init( &gamui2D, look );
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

	label[0].Init( &gamui2D );
	label[0].SetText( "" );
	label[1].Init( &gamui2D );
	label[1].SetText( "Team Tangerine" );

	engine = 0;
	map = 0;

	LoadMap();
}


BattleTestScene::~BattleTestScene()
{
	map->AttachEngine( 0, 0 );
	delete chitBag;
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
	layout.PosAbs( &regionButton, 1, -2 );

	layout.SetSize( LAYOUT_SIZE_X, LAYOUT_SIZE_Y*0.5f );

	int currentGroup = -1;
	int x = 0;
	int col = 0;
	int row = -1;

	for( int i=0; i<NUM_BUTTONS; ++i ) {
		if ( buttonDef[i].group != currentGroup ) {
			x = i < NUM_OPTIONS ? 0 : -4;
			currentGroup = buttonDef[i].group;
			col = 0;
			if ( i == NUM_OPTIONS || i == 0 ) 
				row = 0;
			else
				++row;
		}
		if ( col == 4 ) {
			col = 0;
			++row;
		}
		layout.PosAbs( &optionButton[i], x+col, row+1 );
		++col;
	
	}
	layout.PosAbs( &label[0], 0, 0 );
	layout.PosAbs( &label[1], -4, 0 );
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
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );	

	ChitContext context;
	context.Set( engine, map, game->ToLumosGame() );
	chitBag = new LumosChitBag( context, 0 );

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

	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	map->AttachEngine( engine, chitBag );

	for ( int i=0; i<blocks.Size(); ++i ) {
		const Vector2I& v = blocks[i];
		map->SetRock( v.x, v.y, 1, false, 0 );
	}

	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	//const GameItem& treeItem = itemDefDB->Get( "tree" );

	for( int i=0; i<features.Size(); ++i ) {
		//Chit* chit = chitBag->NewChit();
		const Vector2I& v = features[i];
		map->SetPlant(v.x, v.y, 1+1, 3);
	}

	Vector2I unit = { 2, 16 };
	Vector2I dummy = { 16, 16 };
	CreateChit( unit, HUMAN, PISTOL, LEFT, 0 );
	CreateChit( dummy, DUMMY, NO_WEAPON, MID, 0 );
	dummy.Set( 16, 17 );
	CreateChit( dummy, DUMMY, NO_WEAPON, MID, 0 );

	engine->CameraLookAt( (float)map->Width()/2, (float)map->Height()/2, 
		                  22.f,		// height
						  225.f );	// rotation
}



void BattleTestScene::HandleHotKey( int mask )
{
	if ( mask == GAME_HK_SPACE ) {
		fireTestGun = !fireTestGun;
	}
	else {
		super::HandleHotKey( mask );
	}
}



void BattleTestScene::GoScene()
{
	GLLOG(( "---- GO ---- \n" ));
	battleStarted = true;
	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );

	// Remove everything that is currently on the board that is some sort of character.
	ChitAcceptAll acceptAll;
	chitBag->QuerySpatialHash( &chitArr, b, 0, &acceptAll );
	for( int i=0; i<chitArr.Size(); ++i ) {
		if ( chitArr[i]->GetMoveComponent() ) {
			chitBag->DeleteChit( chitArr[i] );
		}
	}

	static const int LEVEL[4] = { 0, 2, 4, 8 };

	int leftCount	= 1 << ButtonDownID( LEFT_COUNT );
	int leftMoB		= ButtonDownID( LEFT_MOB );
	int leftWeapon	= ButtonDownID( LEFT_WEAPON );
	int leftLevel   = LEVEL[ ButtonDownID( LEFT_LEVEL ) - LEVEL_0  ];
	int rightCount	= 1 << ButtonDownID( RIGHT_COUNT );
	int rightMoB	= ButtonDownID( RIGHT_MOB );
	int rightWeapon = ButtonDownID( RIGHT_WEAPON );
	int rightLoc    = (rightMoB == DUMMY) ? MID : RIGHT;
	int rightLevel   = LEVEL[ ButtonDownID( RIGHT_LEVEL ) - LEVEL_0  ];

	for( int i=0; i<leftCount; ++i ) {
		Chit* c = CreateChit( waypoints[LEFT][i], leftMoB, leftWeapon, LEFT_TEAM, leftLevel );
		if ( i==0 ) {
			c->GetAIComponent()->EnableDebug( true );
		}

	}
	for( int i=0; i<rightCount; ++i ) {
		CreateChit( waypoints[rightLoc][i], rightMoB, rightWeapon, RIGHT_TEAM, rightLevel );
	}

	random.ShuffleArray(waypoints[LEFT].Mem(), waypoints[LEFT].Size());
	random.ShuffleArray(waypoints[RIGHT].Mem(), waypoints[RIGHT].Size());
}

		
Chit* BattleTestScene::CreateChit( const Vector2I& p, int type, int loadout, int team, int level )
{
	const char* itemName = "";
	switch ( type ) {
	case DUMMY:			itemName = "dummyTarget";		break;
	case HUMAN:			itemName = "humanFemale";		break;
	case ARACHNOID:		itemName = "arachnoid";			break;
	case MANTIS:		itemName = "mantis";			break;
	case RED_MANTIS:	itemName = "redMantis";			break;
	case CYCLOPS:		itemName = "cyclops";			break;
	case FIRE_CYCLOPS:	itemName = "fireCyclops";		break;
	case SHOCK_CYCLOPS:	itemName = "shockCyclops";		break;
	case TROLL:			itemName = "troll";				break;
	default: GLASSERT( 0 ); break;
	}

	ItemDefDB::GameItemArr itemDefArr;
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	itemDefDB->Get( itemName, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = chitBag->NewChit();

	chit->Add( new SpatialComponent());
	const char* resourceName = itemDefArr[0]->ResourceName();
	RenderComponent* rc = new RenderComponent( resourceName );
	chit->Add( rc );

	chitBag->AddItem( itemName, chit, engine, team, level );

	if ( type == HUMAN ) {
		chitBag->AddItem( "shield",		chit, engine, 0, level );
		if ( loadout == MELEE_WEAPON || loadout == BLASTER_AND_GUN )
			chitBag->AddItem( "ring",	chit, engine, 0, level );
		if ( loadout == BLASTER || loadout == BLASTER_AND_GUN )
			chitBag->AddItem( "blaster", chit, engine, 0, level );
		else if ( loadout == PISTOL)
			chitBag->AddItem("pistol", chit, engine, 0, level);
		else if (loadout == PULSE)
			chitBag->AddItem("pulse", chit, engine, 0, level);
		else if (loadout == BEAMGUN)
			chitBag->AddItem("beamgun", chit, engine, 0, level);
	}
	if ( type == TROLL ) {
		if ( random.Bit() )
			chitBag->AddItem( "shield",  chit, engine, 0, level );
		if ( random.Bit() )
			chitBag->AddItem( "ring",    chit, engine, 0, level );
		if ( random.Bit() )
			chitBag->AddItem( "blaster", chit, engine, 0, level );
	}

	if ( type != DUMMY ) {
		chit->Add( new PathMoveComponent());
	}
	else {
		// The avoider only avoids things with move components. Makes sense and yet
		// stretches this. (Note: BLOCKS are an entirely different matter. The pather
		// handles those just fine.)
		chit->Add( new MoveComponent());
	}
	AIComponent* ai = new AIComponent();
	ai->SetSectorAwareness( true );
	chit->Add( ai );
	chit->Add( new HealthComponent());
	chit->Add( new DebugStateComponent());

	chit->GetSpatialComponent()->SetPosYRot( (float)p.x+0.5f, 0, (float)p.y+0.5f, (float)random.Rand( 360 ) );

	return chit;
}


void BattleTestScene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();

	micropather::CacheData cacheData;
	Vector2I sector = { 0, 0 };
	map->PatherCacheHitMiss( sector, &cacheData );

	ufoText->Draw( 0, 16, "PathCache mem=%%=%d hit%%=%d chits:ticked/total=%d/%d regions=%d", 
		(int)(cacheData.memoryFraction * 100.0f),
		(int)(cacheData.hitFraction * 100.f),
		chitBag->NumTicked(), chitBag->NumChits(),
		map->CalcNumRegions() );

	if ( debugRay.direction.x ) {
		ModelVoxel mv = engine->IntersectModelVoxel( debugRay.origin, debugRay.direction, 1000.0f, TEST_TRI, 0, 0, 0 );

		if ( mv.Hit() ) {
			engine->particleSystem->EmitPD( "spell", mv.at, V3F_UP, 0 );

			GLString str;
			if ( mv.ModelHit() ) {
				Chit* chit = mv.model->userData;
				if ( chit ) {
					chit->DebugStr( &str );
				}
			}
			else {
				str.Format( "Voxel %d %d %d", mv.voxel.x, mv.voxel.y, mv.voxel.z );
			}

			if ( !str.empty() ) {
				int y = 32;
				ufoText->Draw( 0, y, "%s ", str.c_str() );
				y += 16;
			}
		}
	}
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


void BattleTestScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, engine);
}

void BattleTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		bool tap = Process3DTap( action, view, world, engine );
		if ( action == GAME_TAP_DOWN ) {
#if 1
			Vector3F at;
			IntersectRayPlane( debugRay.origin, debugRay.direction, 1, 0, &at );
			at.y = 0.01f;

			DamageDesc dd( 20, 0 );
			BattleMechanics::GenerateExplosionMsgs( dd, at, 0, engine, chitBag );
#endif		
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
	else if ( item == &regionButton ) {
		Rectangle2I b;
		if ( regionButton.Down() )
			b.Set( 0, 0, map->Width()-1, map->Height()-1 );
		else
			b.Set( 0, 0, 0, 0 );
		map->ShowRegionOverlay( b );
	}
}


void BattleTestScene::DoTick( U32 deltaTime )
{
	// Battle Bolt test!
	if ( fireTestGun ) {
		boltTimer += deltaTime;
		if ( boltTimer > 500 ) {
			boltTimer = 0;
			ChitBag* cb = chitBag;
			Bolt* bolt = cb->NewBolt();

			bolt->dir.Set( -0.1f+fuzz.Uniform()*0.2f, 0, 1 );
			bolt->head.Set( 0.5f * (float)map->Width(), 1, 2 );
			bolt->color.Set( 1, 0.1f, 0.3f, 1 );
			bolt->damage = 20.0f;
			bolt->effect = GameItem::EFFECT_FIRE;
			bolt->speed = 10.0f;
		}
	}
	map->DoTick(deltaTime, chitBag);
	chitBag->DoTick( deltaTime );
}


void BattleTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime, chitBag->BoltMem(), chitBag->NumBolts() );
}

