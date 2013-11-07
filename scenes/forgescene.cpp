#include "forgescene.h"
#include "../game/lumosgame.h"
#include "../game/gameitem.h"
#include "../engine/engine.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"

using namespace gamui;
using namespace grinliz;

static const float NEAR = 0.1f;
static const float FAR  = 10.0f;


ForgeSceneData::~ForgeSceneData()
{
	delete item;
}


ForgeScene::ForgeScene( LumosGame* game, ForgeSceneData* data ) : Scene( game ), lumosGame( game ), screenport( game->GetScreenport())
{
	forgeData = data;
	forgeData->item = new GameItem();
	itemBuilt = false;

	screenport.SetNearFar( NEAR, FAR );
	engine = new Engine( &screenport, lumosGame->GetDatabase(), 0 );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	engine->lighting.direction.Set( 2, 1, 1 );
	engine->lighting.direction.Normalize();
	
	model = 0;

	static const float D = 3; //1.5f;
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	engine->camera.SetPosWC( D, 0, 0 );
	engine->CameraLookAt( engine->camera.PosWC(), V3F_ZERO );

	lumosGame->InitStd( &gamui2D, &okay, 0 );

	availableLabel.Init( &gamui2D );
	requiredLabel.Init( &gamui2D );
	techAvailLabel.Init( &gamui2D );
	techRequiredLabel.Init( &gamui2D );

	for( int i=0; i<NUM_ITEM_TYPES; ++i ) {
		static const char* name[NUM_ITEM_TYPES] = { "Ring", "Gun", "Shield" };
		itemType[i].Init( &gamui2D, game->GetButtonLook(0));
		itemType[i].SetText( name[i] );
		itemType[0].AddToToggleGroup( &itemType[i] );
	}
	for( int i=0; i<NUM_GUN_TYPES; ++i ) {
		static const char* name[NUM_GUN_TYPES] = { "Pistol", "Blaster", "Pulse", "Beamgun" };
		gunType[i].Init( &gamui2D, game->GetButtonLook(0));
		gunType[i].SetText( name[i] );
		gunType[0].AddToToggleGroup( &gunType[i] );
	}
	for( int i=0; i<NUM_GUN_PARTS; ++i ) {
		static const char* name[NUM_GUN_PARTS] = { "Body", "Cell", "Driver", "Scope" };
		gunParts[i].Init( &gamui2D, game->GetButtonLook(0));
		gunParts[i].SetText( name[i] );
	}
	for( int i=0; i<NUM_RING_PARTS; ++i ) {
		static const char* name[NUM_RING_PARTS] = { "Main", "Guard", "Triad", "Blade" };
		ringParts[i].Init( &gamui2D, game->GetButtonLook(0));
		ringParts[i].SetText( name[i] );
	}
	for( int i=0; i<NUM_EFFECTS; ++i ) {
		static const char* name[NUM_EFFECTS] = { "Fire", "Shock", "Explosive" };
		effects[i].Init( &gamui2D, game->GetButtonLook(0));
		effects[i].SetText( name[i] );
	}
	itemDescWidget.Init( &gamui2D );
	crystalRequiredWidget.Init( &gamui2D );
	crystalAvailWidget.Init( &gamui2D );

	availableLabel.SetText( "Available:" );
	requiredLabel.SetText( "Required:" );

	CStr<64> str;
	str.Format( "Tech: %d", data->tech );
	techAvailLabel.SetText( str.c_str() );

	crystalAvailWidget.Set( data->wallet );

	buildButton.Init( &gamui2D, game->GetButtonLook(0));
	buildButton.SetText( "Build" );

	SetModel();
}


ForgeScene::~ForgeScene()
{
	if ( !itemBuilt ) {
		delete forgeData->item;
		forgeData->item = 0;
	}
	engine->FreeModel( model );
	delete engine;
}


void ForgeScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
	LayoutCalculator layout = lumosGame->DefaultLayout();

	for( int i=0; i<NUM_ITEM_TYPES; ++i ) {
		layout.PosAbs( &itemType[i], 0, i );		
	}
	for( int i=0; i<NUM_GUN_TYPES; ++i ) {
		layout.PosAbs( &gunType[i], 1, i );		
	}
	gunParts[0].SetVisible( false );
	ringParts[0].SetVisible( false );

	for( int i=1; i<NUM_GUN_PARTS; ++i ) {
		layout.PosAbs( &gunParts[i], 2, i-1 );
	}
	for( int i=1; i<NUM_RING_PARTS; ++i ) {
		layout.PosAbs( &ringParts[i], 2, i-1 );
	}

	for( int i=0; i<NUM_EFFECTS; ++i ) {
		layout.PosAbs( &effects[i], 3, i );
	}

	// --- Half vertical -- //
	layout.PosAbs( &availableLabel,			0, NUM_ITEM_TYPES+1 );
	layout.PosAbs( &techAvailLabel,			1, NUM_ITEM_TYPES+1 );
	layout.PosAbs( &crystalAvailWidget,		1, NUM_ITEM_TYPES+2 );
	layout.PosAbs( &requiredLabel,			0, NUM_ITEM_TYPES+3 );
	layout.PosAbs( &techRequiredLabel,		1, NUM_ITEM_TYPES+3 );
	layout.PosAbs( &crystalRequiredWidget,	1, NUM_ITEM_TYPES+4 );

	itemDescWidget.SetLayout( layout );
	layout.PosAbs( &itemDescWidget, -4, 0 );

	layout.PosAbs( &buildButton, -2, NUM_ITEM_TYPES+1, 2, 2 );
}


void ForgeScene::SetModel()
{
	GameItem humanMale = ItemDefDB::Instance()->Get( "humanMale" );
	techRequired = 0;
	crystalRequired.EmptyWallet();
	crystalRequired.crystal[CRYSTAL_GREEN] += 1;

	int type = RING;
	if ( itemType[GUN].Down() )		type = GUN;
	if ( itemType[SHIELD].Down() )	type = SHIELD;
	
	for( int i=0; i<NUM_GUN_TYPES; ++i ) {
		gunType[i].SetVisible( type == GUN );
	}
	for( int i=1; i<NUM_GUN_PARTS; ++i ) {
		gunParts[i].SetVisible( type == GUN );
	}
	for( int i=1; i<NUM_RING_PARTS; ++i ) {
		ringParts[i].SetVisible( type == RING );
	}

	const char* typeName = "pistol";

	int roll[GameTrait::NUM_TRAITS] = { 10, 10, 10, 10, 10 };

	static const int BONUS = 2;

	int features = 0;
	if ( type == GUN ) {
		if ( gunType[BLASTER].Down() )		{ typeName = "blaster"; }
		else if ( gunType[PULSE].Down() )	{ typeName = "pulse";   techRequired += 1; }
		else if ( gunType[BEAMGUN].Down() ) { typeName = "beamgun"; techRequired += 1; }

		if ( gunParts[GUN_CELL].Down() )		{ 
			features |= WeaponGen::GUN_CELL;		
			roll[GameTrait::ALT_CAPACITY] += BONUS*2; 
			techRequired++;
		}
		if ( gunParts[GUN_DRIVER].Down() )		{ 
			features |= WeaponGen::GUN_DRIVER;	
			roll[GameTrait::ALT_DAMAGE]   += BONUS;	
			techRequired++;
		}
		if ( gunParts[GUN_SCOPE].Down() )		{ 
			features |= WeaponGen::GUN_SCOPE;		
			roll[GameTrait::ALT_ACCURACY] += BONUS; 
			techRequired++;
		}
	}
	else if ( type == RING ) {
		typeName = "ring";

		if ( ringParts[RING_GUARD].Down() )		{ 
			features |= WeaponGen::RING_GUARD;	
			roll[GameTrait::CHR] += BONUS; 
			techRequired++;
		}	
		if ( ringParts[RING_TRIAD].Down() )		{ 
			features |= WeaponGen::RING_TRIAD;	
			roll[GameTrait::ALT_DAMAGE] += BONUS/2; 
			techRequired++;
		}
		if ( ringParts[RING_BLADE].Down() )		{ 
			features |= WeaponGen::RING_BLADE;	
			roll[GameTrait::CHR] -= BONUS; 
			roll[GameTrait::ALT_DAMAGE] += BONUS; 
		}	
	}
	else if ( type == SHIELD ) {
		typeName = "shield";
	}

	if ( !itemBuilt ) {
		const GameItem& item = ItemDefDB::Instance()->Get( typeName );
		forgeData->item->CopyFrom( &item, forgeData->item->id );
		ItemDefDB::Instance()->AssignWeaponStats( roll, item, forgeData->item );
	}
	itemDescWidget.SetInfo( forgeData->item, &humanMale );

	int eff = 0;
	if ( effects[EFFECT_FIRE].Down() )		eff |= GameItem::EFFECT_FIRE;
	if ( effects[EFFECT_SHOCK].Down() )		eff |= GameItem::EFFECT_SHOCK;
	if ( effects[EFFECT_EXPLOSIVE].Down() )	eff |= GameItem::EFFECT_EXPLOSIVE;

	if ( eff & GameItem::EFFECT_FIRE ) {
		crystalRequired.crystal[CRYSTAL_RED] += 1;
	}
	if ( eff & GameItem::EFFECT_SHOCK ) {
		crystalRequired.crystal[CRYSTAL_BLUE] += 1;
	}
	if ( eff & GameItem::EFFECT_EXPLOSIVE ) {
		crystalRequired.crystal[CRYSTAL_VIOLET] += 1;
	}
	// 2 effects adds a 2nd violet crystal
	if ( FloorPowerOf2( eff ) != eff ) {
		crystalRequired.crystal[CRYSTAL_VIOLET] += 1;
	}

	engine->FreeModel( model );
	model = engine->AllocModel( typeName );
	model->SetPos( 0, 0, 0 );

	// Rotate the shield to face the camera.
	Quaternion q;
	static const Vector3F AXIS = { 0, 0, 1 };
	q.FromAxisAngle( AXIS, type == SHIELD ? -90.0f : 0.0f );
	model->SetRotation( q );

	ProcRenderInfo info;
	AssignProcedural( typeName, false, forgeData->item->id, 0, false, eff, features, &info );

	model->SetTextureXForm( info.te.uvXForm.x, info.te.uvXForm.y, info.te.uvXForm.z, info.te.uvXForm.w );
	model->SetTextureClip( info.te.clip.x, info.te.clip.y, info.te.clip.z, info.te.clip.w );
	model->SetColorMap( true, info.color );
	model->SetBoneFilter( info.filterName, info.filter );

	CStr<64> str;
	str.Format( "Tech: %d", techRequired );
	techRequiredLabel.SetText( str.c_str() );
	crystalRequiredWidget.Set( crystalRequired );
	
	if (    !itemBuilt 
		 && techRequired <= forgeData->tech 
		 && crystalRequired <= forgeData->wallet ) 
	{
		buildButton.SetEnabled( true );
	}
	else {
		buildButton.SetEnabled( false );
	}
}


void ForgeScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
	else if ( item == &buildButton ) {
		// Can only be tapped if enabled.
		forgeData->wallet.Remove( crystalRequired );
		forgeData->itemWallet.Add( crystalRequired );	// becomes part of the item, and will be returned to Reserve when item is destroyed.
		forgeData->item->traits.Roll( forgeData->item->id, 
									  GameTrait::NUM_TRAITS*2 + forgeData->level );

		crystalAvailWidget.Set( forgeData->wallet );
		crystalRequiredWidget.SetVisible( false );

		// Disable everything.
		for( int i=0; i<NUM_ITEM_TYPES; ++i ) {
			itemType[i].SetEnabled(false);		
		}
		for( int i=0; i<NUM_GUN_TYPES; ++i ) {
			gunType[i].SetEnabled( false );
		}
		for( int i=1; i<NUM_GUN_PARTS; ++i ) {
			gunParts[i].SetEnabled( false );
		}
		for( int i=1; i<NUM_RING_PARTS; ++i ) {
			ringParts[i].SetEnabled( false );
		}
		for( int i=0; i<NUM_EFFECTS; ++i ) {
			effects[i].SetEnabled( false );
		}
		itemBuilt = true;
	}
	SetModel();
}


void ForgeScene::Draw3D( U32 deltaTime )
{
	// we use our own screenport
	screenport.SetPerspective();
	engine->Draw( deltaTime, 0, 0 );
	screenport.SetUI();
}




