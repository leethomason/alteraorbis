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
#include "../game/team.h"

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
	{ TRILOBYTE,"Trilobyte",		LEFT_MOB },
	{ MANTIS,	"Mantis",			LEFT_MOB },
	{ RED_MANTIS,"RedMantis",		LEFT_MOB },
	{ CYCLOPS,	"Cyclops",			LEFT_MOB },
	{ FIRE_CYCLOPS,	"F-Cyclops",	LEFT_MOB },
	{ SHOCK_CYCLOPS,	"S-Cyclops",LEFT_MOB },
	{ TROLL,	"Troll",	LEFT_MOB },
	{ GOBMAN,	"Gobman",	LEFT_MOB },
	{ KAMAKIRI,	"Kamakiri",	LEFT_MOB },
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
	{ TRILOBYTE,"Trilobyte",		RIGHT_MOB },
	{ MANTIS,	"Mantis",			RIGHT_MOB },
	{ RED_MANTIS,	"RedMantis",	RIGHT_MOB },
	{ CYCLOPS,	"Cyclops",			RIGHT_MOB },
	{ FIRE_CYCLOPS,	"F-Cyclops",	RIGHT_MOB },
	{ SHOCK_CYCLOPS,	"S-Cyclops",RIGHT_MOB },
	{ TROLL,	"Troll",	RIGHT_MOB },
	{ GOBMAN,	"Gobman",	RIGHT_MOB },
	{ KAMAKIRI,	"Kamakiri",	RIGHT_MOB },
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

	context.game = game;
	new Team(game->GetDatabase());
	InitStd( &gamui2D, &okay, 0 );
	LayoutCalculator layout = DefaultLayout();

	const ButtonLook& look = context.game->GetButtonLook( LumosGame::BUTTON_LOOK_STD );

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

	context.engine = 0;
	context.worldMap = 0;

	LoadMap();
}


BattleTestScene::~BattleTestScene()
{
	context.worldMap->AttachEngine( 0, 0 );
	delete context.chitBag;
	delete context.engine;
	delete context.worldMap;
	delete Team::Instance();
}


void BattleTestScene::Resize()
{
//	LumosGame* lumosGame = static_cast<LumosGame*>( game );

//	const Screenport& port = context.game->GetScreenport();
	LayoutCalculator layout = DefaultLayout();

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
		layout.PosAbs( &optionButton[i], x+col, row );
		++col;
	
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
	delete context.engine;
	delete context.worldMap;

	context.worldMap = new WorldMap( 32, 32 );
	context.engine = new Engine( context.game->GetScreenportMutable(), context.game->GetDatabase(), context.worldMap );	

	context.chitBag = new LumosChitBag( context, 0 );

	grinliz::CDynArray<Vector2I> blocks, features, wp;
	context.worldMap->InitPNG( "./res/testarena32.png", &blocks, &wp, &features );

	for( int i=0; i<wp.Size(); ++i ) {
		Vector2I v = wp[i];
		if ( v.x < context.worldMap->Width() / 3 ) {
			waypoints[LEFT].Push( v );
		}
		else if ( v.x > context.worldMap->Width()*2/3 ) {
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

	context.engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	context.worldMap->AttachEngine( context.engine, context.chitBag );

	for ( int i=0; i<blocks.Size(); ++i ) {
		const Vector2I& v = blocks[i];
		context.worldMap->SetRock( v.x, v.y, 1, false, 0 );
	}

//	ItemDefDB* itemDefDB = ItemDefDB::Instance();

	for( int i=0; i<features.Size(); ++i ) {
		const Vector2I& v = features[i];
		context.worldMap->SetPlant(v.x, v.y, 1+1, 3);
	}

	Vector2I unit = { 2, 16 };
	Vector2I dummy = { 16, 16 };
	CreateChit( unit, HUMAN, PISTOL, Team::CombineID(TEAM_HOUSE, TEAM_ID_LEFT), 0 );
	CreateChit( dummy, DUMMY, NO_WEAPON, Team::CombineID(TEAM_HOUSE, TEAM_ID_RIGHT), 0 );
	dummy.Set( 16, 17 );
	CreateChit( dummy, DUMMY, NO_WEAPON, Team::CombineID(TEAM_HOUSE, TEAM_ID_RIGHT), 0 );

	context.engine->CameraLookAt( (float)context.worldMap->Width()/2, (float)context.worldMap->Height()/2, 
		                  22.f,		// height
						  225.f );	// rotation
}



void BattleTestScene::HandleHotKey( int mask )
{
	if ( mask == GAME_HK_DEBUG_ACTION ) {
		fireTestGun = !fireTestGun;
	}
	else {
		super::HandleHotKey( mask );
	}
}



void BattleTestScene::GoScene()
{
//	GLLOG(( "---- GO ---- \n" ));
	battleStarted = true;
	Rectangle2F b;
	b.Set( 0, 0, (float)context.worldMap->Width(), (float)context.worldMap->Height() );

	// Remove everything that is currently on the board that is some sort of character.
	ChitAcceptAll acceptAll;
	context.chitBag->QuerySpatialHash( &chitArr, b, 0, &acceptAll );
	for( int i=0; i<chitArr.Size(); ++i ) {
		if ( chitArr[i]->GetMoveComponent() ) {
			context.chitBag->DeleteChit( chitArr[i] );
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
		Chit* c = CreateChit( waypoints[LEFT][i], leftMoB, leftWeapon, Team::CombineID(TEAM_HOUSE, TEAM_ID_LEFT), leftLevel );
		if ( i==0 ) {
			if (c->GetAIComponent()) {
				c->GetAIComponent()->EnableLog(true);
				c->GetItemComponent()->EnableDebug(true);
			}
		}

	}
	for( int i=0; i<rightCount; ++i ) {
		CreateChit( waypoints[rightLoc][i], rightMoB, rightWeapon, Team::CombineID(TEAM_HOUSE, TEAM_ID_RIGHT), rightLevel );
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
	case TRILOBYTE:		itemName = "trilobyte";			break;
	case MANTIS:		itemName = "mantis";			break;
	case RED_MANTIS:	itemName = "redMantis";			break;
	case CYCLOPS:		itemName = "cyclops";			break;
	case FIRE_CYCLOPS:	itemName = "fireCyclops";		break;
	case SHOCK_CYCLOPS:	itemName = "shockCyclops";		break;
	case TROLL:			itemName = "troll";				break;
	case GOBMAN:		itemName = "gobman";			break;
	case KAMAKIRI:		itemName = "kamakiri";			break;
	default: GLASSERT( 0 ); break;
	}

	ItemDefDB::GameItemArr itemDefArr;
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	itemDefDB->Get( itemName, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	Chit* chit = context.chitBag->NewChit();

	const char* resourceName = itemDefArr[0]->ResourceName();
	RenderComponent* rc = new RenderComponent( resourceName );
	chit->Add( rc );

	GameItem* mobItem = context.chitBag->AddItem( itemName, chit, context.engine, team, level );

	if ( mobItem->IsDenizen() || type == TROLL) {
		context.chitBag->AddItem( "shield",		chit, context.engine, 0, level );
		if ( loadout == MELEE_WEAPON || loadout == BLASTER_AND_GUN )
			context.chitBag->AddItem( "ring",	chit, context.engine, 0, level );
		if ( loadout == BLASTER || loadout == BLASTER_AND_GUN )
			context.chitBag->AddItem( "blaster", chit, context.engine, 0, level );
		else if ( loadout == PISTOL)
			context.chitBag->AddItem("pistol", chit, context.engine, 0, level);
		else if (loadout == PULSE)
			context.chitBag->AddItem("pulse", chit, context.engine, 0, level);
		else if (loadout == BEAMGUN)
			context.chitBag->AddItem("beamgun", chit, context.engine, 0, level);
	}

	if ( type != DUMMY ) {
		chit->Add( new PathMoveComponent());
		AIComponent* ai = new AIComponent();
		ai->SetSectorAwareness( true );
		chit->Add( ai );
	}
	else {
		// The avoider only avoids things with move components. Makes sense and yet
		// stretches this. (Note: BLOCKS are an entirely different matter. The pather
		// handles those just fine.)
		chit->Add( new GameMoveComponent());
	}
	chit->Add( new HealthComponent());
	chit->Add( new DebugStateComponent());

	chit->SetPosition((float)p.x + 0.5f, 0, (float)p.y + 0.5f);
	Quaternion q = Quaternion::MakeYRotation(float(random.Rand(360)));
	chit->SetRotation(q);

	return chit;
}


void BattleTestScene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();

	micropather::CacheData cacheData;
	Vector2I sector = { 0, 0 };
	context.worldMap->PatherCacheHitMiss( sector, &cacheData );

	ufoText->Draw( 0, 16, "PathCache mem=%%=%d hit%%=%d chits:ticked/total=%d/%d regions=%d", 
		(int)(cacheData.memoryFraction * 100.0f),
		(int)(cacheData.hitFraction * 100.f),
		context.chitBag->NumTicked(), context.chitBag->NumChits(),
		context.worldMap->CalcNumRegions() );

	if ( debugRay.direction.x ) {
		ModelVoxel mv = context.engine->IntersectModelVoxel( debugRay.origin, debugRay.direction, 1000.0f, TEST_TRI, 0, 0, 0 );

		if ( mv.Hit() ) {
			const ParticleDef& spell = context.engine->particleSystem->GetPD("spell");
			context.engine->particleSystem->EmitPD( spell, mv.at, V3F_UP, 0 );

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
		context.engine->SetZoom( context.engine->GetZoom() *( 1.0f+delta) );
	else
		context.engine->SetZoom( context.engine->GetZoom() + delta );
}


void BattleTestScene::Rotate( float degrees ) 
{
	context.engine->camera.Orbit( degrees );
}


void BattleTestScene::MoveCamera(float dx, float dy)
{
	MoveImpl(dx, dy, context.engine);
}

bool BattleTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		Process3DTap( action, view, world, context.engine );
		if ( action == GAME_TAP_DOWN ) {
#if 1
			Vector3F at;
			IntersectRayPlane( debugRay.origin, debugRay.direction, 1, 0, &at );
			at.y = 0.01f;

			DamageDesc dd( 20, 0 );
			BattleMechanics::GenerateExplosion( dd, at, 0, context.engine, context.chitBag, context.worldMap);
#endif		
		}
	}
	return true;
}


void BattleTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		context.game->PopScene();
	}
	else if ( item == &goButton ) {
		GoScene();
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


void BattleTestScene::DoTick( U32 deltaTime )
{
	// Battle Bolt test!
	if ( fireTestGun ) {
		boltTimer += deltaTime;
		if ( boltTimer > 500 ) {
			boltTimer = 0;
			ChitBag* cb = context.chitBag;
			Bolt* bolt = cb->NewBolt();

			bolt->dir.Set( -0.1f+fuzz.Uniform()*0.2f, 0, 1 );
			bolt->head.Set( 0.5f * (float)context.worldMap->Width(), 1, 2 );
			bolt->color.Set( 1, 0.1f, 0.3f, 1 );
			bolt->damage = 20.0f;
			bolt->effect = GameItem::EFFECT_FIRE;
			bolt->speed = 10.0f;
		}
	}
	context.worldMap->DoTick(deltaTime, context.chitBag);
	context.chitBag->DoTick( deltaTime );
}


void BattleTestScene::Draw3D( U32 deltaTime )
{
	context.engine->Draw( deltaTime, context.chitBag->BoltMem(), context.chitBag->NumBolts() );
}

