#pragma warning (disable:4530)
#include "../shared/lodepng.h"

#include "livepreviewscene.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../xegame/testmap.h"

#include "../game/lumosgame.h"
#include "../game/gameitem.h"
#include "../script/procedural.h"

#include <sys/stat.h>

using namespace gamui;
using namespace grinliz;

LivePreviewScene::LivePreviewScene( LumosGame* game, const LivePreviewSceneData* data ) : Scene( game )
{
	TestMap* map = 0;
	timer = 0;
	
	//map = new TestMap( 8, 8 );
	//Color3F c = { 0.5f, 0.5f, 0.5f };
	//map->SetColor( c );
	
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	LayoutCalculator layout = game->DefaultLayout();
	const ButtonLook& look = game->GetButtonLook(0);

	for( int i=0; i<ROWS; ++i ) {
		rowButton[i].Init( &gamui2D, look );
		CStr< 16 > t;
		t.Format( "%d", i+1 );
		rowButton[i].SetText( t.c_str() );
		rowButton[0].AddToToggleGroup( &rowButton[i] );
	}

	electricButton.Init( &gamui2D, look );
	electricButton.SetText( "Electric" );

	static const char* typeName[NUM_TYPES] = { "Male\nFace", "Female\nFace", "Ring", "Gun" };
	for( int i=0; i<NUM_TYPES; ++i ) {
		typeButton[i].Init( &gamui2D, look );
		typeButton[i].SetText( typeName[i] );
		typeButton[0].AddToToggleGroup( &typeButton[i] );
	}

	memset( model, 0, sizeof(model[0])*NUM_MODEL );

	currentType = HUMAN_MALE_FACE;
	GenerateAndCreate();
	game->InitStd( &gamui2D, &okay, 0 );
}


LivePreviewScene::~LivePreviewScene()
{
	Map* map = engine->GetMap();
	for( int i=0; i<NUM_MODEL; ++i ) 
		engine->FreeModel( model[i] );
	delete engine;
	delete map;
}


void LivePreviewScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, 0 );

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	for( int i=0; i<ROWS; ++i ) {
		layout.PosAbs( &rowButton[i], 0, i );
	}
	for( int i=0; i<NUM_TYPES; ++i ) {
		layout.PosAbs( &typeButton[i], -1, i );
	}
	layout.PosAbs( &electricButton, 0, 4 );
}


grinliz::Color4F LivePreviewScene::ClearColor()
{
	//Color4F cc = game->GetPalette(0)->Get4F( PAL_ZERO, PAL_GRAY );
	Color4F cc = { 0.2f, 0.2f, 0.2f, 0.0f };
	//Color4F cc = { 0,0,0,0 };
	return cc;
}


void LivePreviewScene::GenerateFaces( int mainRow )
{
	static const float CENTER_X = 4.0f;
	static const float CENTER_Z = 4.0f;
	static const float START_X = 2.0f;
	static const float START_Z = 2.5f;

	engine->lighting.direction.Set( 0, 1, 0 );
	engine->camera.SetPosWC( CENTER_X, 11, CENTER_Z );
	static const Vector3F out = { 0, 0, 1 };
	engine->camera.SetDir( V3F_DOWN, out );		// look straight down. This works! cool.
	engine->camera.Orbit( 180.0f );

	//FaceGen faceGen( currentType == HUMAN_FEMALE_FACE );

	Random random( mainRow );
	random.Rand();

	static const char* resName[2] = { "unitPlateHumanMaleFace", "unitPlateHumanFemaleFace" };
	GLASSERT( currentType >= 0 && currentType < 2 );

	const ModelResource* modelResource = 0;
	modelResource = ModelResourceManager::Instance()->GetModelResource( resName[currentType] );

	int srcRows = modelResource->atom[0].texture->Height() / modelResource->atom[0].texture->Width() * 4;
	float rowMult = 1.0f / (float)srcRows;

	for( int i=0; i<NUM_MODEL; ++i ) {
		if ( model[i] ) {
			engine->FreeModel( model[i] );
			model[i] = 0;
		}

		model[i] = engine->AllocModel( modelResource );
		int row = i / COLS;
		int col = i - row*COLS;
		float x = START_X + float(col);
		float z = START_Z + float(row);
		
		model[i]->SetPos( x, 0.1f, z );
		
		HumanGen gen( currentType == HUMAN_FEMALE_FACE, random.Rand(), 1, electricButton.Down() );
		ProcRenderInfo info;
		gen.AssignFace( &info );

		model[i]->SetTextureXForm( info.te.uvXForm.x, info.te.uvXForm.y, info.te.uvXForm.z, info.te.uvXForm.w );
		model[i]->SetTextureClip( info.te.clip.x, info.te.clip.y, info.te.clip.z, info.te.clip.w );
		model[i]->SetColorMap( true, info.color );
	}
}


void LivePreviewScene::GenerateRingOrGun( int mainRow, bool gun )
{
	static const float DELTA  = 0.3f;
	static const float DIST   = 3.0f;

	engine->lighting.direction.Set( -1, 1, -1 );
	engine->lighting.direction.Normalize();
	
	engine->camera.SetPosWC( 0, DELTA*1.5f, DELTA*(float)(COLS-1)*0.5f );
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	Random random( mainRow );
	random.Rand();

	static const char* GUN_NAMES[4] = { "pistol", "blaster", "beamgun", "pulse" };

	const char* resName = "ring";
	if ( gun ) {
		resName = GUN_NAMES[mainRow%4];
	}

	const ModelResource* modelResource = 0;

	modelResource = ModelResourceManager::Instance()->GetModelResource( resName );

	int srcRows = modelResource->atom[0].texture->Height() / modelResource->atom[0].texture->Width() * 4;
	float rowMult = 1.0f / (float)srcRows;

	for( int i=0; i<NUM_MODEL; ++i ) {
		if ( model[i] ) {
			engine->FreeModel( model[i] );
			model[i] = 0;
		}

		model[i] = engine->AllocModel( modelResource );
		int row = i / COLS;
		int col = i - row*COLS;
		float x = float(col) * DELTA;
		float y = float(ROWS-1-row) * DELTA;
		float current = 1.0f - rowMult * (float)(mainRow);

		static const int FLAGS[COLS] = { 0, GameItem::EFFECT_FIRE, GameItem::EFFECT_FIRE | GameItem::EFFECT_EXPLOSIVE, GameItem::EFFECT_SHOCK, GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK };

		WeaponGen weaponGen( i + mainRow*137, FLAGS[col] );
		ProcRenderInfo info;
		if ( gun )
			weaponGen.AssignGun( &info );
		else
			weaponGen.AssignRing( &info );

		if ( i == NUM_MODEL-1 ) {
			model[i]->SetPos( 3.0f, y, x+0.15f );
		}
		else {
			model[i]->SetPos( 3.0f, y, x );
		}
		model[i]->SetTextureXForm( info.te.uvXForm );
		model[i]->SetColorMap( true, info.color );
		model[i]->SetBoneFilter( info.filterName, info.filter );
	}
}


void LivePreviewScene::GenerateAndCreate()
{
	for( int i=0; i<ROWS; ++i ) {
		if ( rowButton[i].Down() ) {
			switch ( currentType ) {
				case		HUMAN_MALE_FACE:		GenerateFaces( i );		break;
				case		HUMAN_FEMALE_FACE:		GenerateFaces( i );		break;
				case		RING:					GenerateRingOrGun( i, false );		break;
				case		GUN:					GenerateRingOrGun( i, true );		break;
				default:	GLASSERT( 0 );									break;
			}
			break;
		}
	}
}


void LivePreviewScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &typeButton[HUMAN_MALE_FACE] ) {
		currentType = HUMAN_MALE_FACE;
		GenerateAndCreate(  );
	}
	else if ( item == &typeButton[HUMAN_FEMALE_FACE] ) {
		currentType = HUMAN_FEMALE_FACE;
		GenerateAndCreate( );
	}
	else if ( item == &typeButton[RING] ) {
		currentType = RING;
		GenerateAndCreate();
	}
	else if ( item == &typeButton[GUN] ) {
		currentType = GUN;
		GenerateAndCreate();
	}

	for( int i=0; i<ROWS; ++i ) {
		if ( item == &rowButton[i] ) {
			GenerateAndCreate();
		}
	}
	if ( item == &electricButton ) {
		GenerateAndCreate();
	}
}


void LivePreviewScene::Draw3D( U32 deltaTime )
{
	timer += deltaTime;
	if ( currentType > HUMAN_FEMALE_FACE ) {
		model[NUM_MODEL-1]->SetYRotation( (float)((timer/20)%360) );
	}
	engine->Draw( deltaTime );
}


void LivePreviewScene::DrawDebugText()
{
	DrawDebugTextDrawCalls( 16, engine );
}
