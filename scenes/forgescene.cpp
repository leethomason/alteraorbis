#include "forgescene.h"
#include "../game/lumosgame.h"
#include "../game/gameitem.h"
#include "../game/news.h"
#include "../game/lumoschitbag.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"

#include "../engine/engine.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"

using namespace gamui;
using namespace grinliz;

static const float NEAR = 0.1f;
static const float FAR  = 10.0f;


ForgeScene::ForgeScene( LumosGame* game, ForgeSceneData* data ) 
	:	Scene( game ), 
		lumosGame( game ), 
		screenport( game->GetScreenport())
{
	forgeData = data;
	item = new GameItem();
	random.SetSeed( item->ID() );
	random.Rand();
	random.Rand();

	screenport.SetNearFar( NEAR, FAR );
	engine = new Engine( &screenport, lumosGame->GetDatabase(), 0 );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	engine->lighting.direction.Set( 2, 1, 1 );
	engine->lighting.direction.Normalize();
	
	model = 0;

	static const float D = 3; //1.5f;
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	engine->camera.SetPosWC( D, 0, 0 );
	engine->CameraLookAt( engine->camera.PosWC(), V3F_ZERO );

	InitStd( &gamui2D, &okay, 0 );

	availableLabel.Init( &gamui2D );
	requiredLabel.Init( &gamui2D );
	techAvailLabel.Init( &gamui2D );
	techRequiredLabel.Init( &gamui2D );
	log.Init(&gamui2D);

	for( int i=0; i<ForgeScript::NUM_ITEM_TYPES; ++i ) {
		itemType[i].Init( &gamui2D, game->GetButtonLook(0));
		itemType[i].SetText( ForgeScript::ItemName(i) );
		itemType[0].AddToToggleGroup( &itemType[i] );
	}
	for( int i=0; i<ForgeScript::NUM_GUN_TYPES; ++i ) {
		gunType[i].Init( &gamui2D, game->GetButtonLook(0));
		gunType[i].SetText( ForgeScript::GunType(i) );
		gunType[0].AddToToggleGroup( &gunType[i] );
	}
	for( int i=0; i<NUM_GUN_PARTS; ++i ) {
		gunParts[i].Init( &gamui2D, game->GetButtonLook(0));
		gunParts[i].SetText( ForgeScript::ItemPart( ForgeScript::GUN, i ));
	}
	for( int i=0; i<NUM_RING_PARTS; ++i ) {
		ringParts[i].Init( &gamui2D, game->GetButtonLook(0));
		ringParts[i].SetText( ForgeScript::ItemPart( ForgeScript::RING, i ));
	}
	for( int i=0; i<ForgeScript::NUM_EFFECTS; ++i ) {
		effects[i].Init( &gamui2D, game->GetButtonLook(0));
		effects[i].SetText( ForgeScript::Effect(i) );
	}
	itemDescWidget.Init( &gamui2D );
	crystalRequiredWidget.Init( &gamui2D );
	crystalAvailWidget.Init( &gamui2D );

	availableLabel.SetText( "Available:" );
	requiredLabel.SetText( "Required:" );

	CStr<64> str;
	str.Format( "Tech: %d", data->tech );
	techAvailLabel.SetText( str.c_str() );

	crystalAvailWidget.Set( forgeData->itemComponent->GetItem(0)->wallet );

	buildButton.Init( &gamui2D, game->GetButtonLook(0));
	buildButton.SetText( "Forge" );

	SetModel( true );
}


ForgeScene::~ForgeScene()
{
	delete item;
	engine->FreeModel( model );
	delete engine;
}


void ForgeScene::Resize()
{
	PositionStd( &okay, 0 );
	LayoutCalculator layout = DefaultLayout();

	for( int i=0; i<ForgeScript::NUM_ITEM_TYPES; ++i ) {
		layout.PosAbs( &itemType[i], 0, i );		
	}
	for( int i=0; i<ForgeScript::NUM_GUN_TYPES; ++i ) {
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

	for( int i=0; i<ForgeScript::NUM_EFFECTS; ++i ) {
		layout.PosAbs( &effects[i], 3, i );
	}

	// --- Half vertical -- //
	layout.PosAbs( &availableLabel,			0, ForgeScript::NUM_ITEM_TYPES+1 );
	layout.PosAbs( &techAvailLabel,			1, ForgeScript::NUM_ITEM_TYPES+1 );
	layout.PosAbs( &crystalAvailWidget,		1, ForgeScript::NUM_ITEM_TYPES+2 );
	layout.PosAbs( &requiredLabel,			0, ForgeScript::NUM_ITEM_TYPES+3 );
	layout.PosAbs( &techRequiredLabel,		1, ForgeScript::NUM_ITEM_TYPES+3 );
	layout.PosAbs( &crystalRequiredWidget,	1, ForgeScript::NUM_ITEM_TYPES+4 );

	itemDescWidget.SetLayout( layout );
	layout.PosAbs( &itemDescWidget, -4, 0 );

	layout.PosAbs( &buildButton, 0, ForgeScript::NUM_ITEM_TYPES+5, 2, 2 );
	layout.PosAbs(&log, -4, ForgeScript::NUM_ITEM_TYPES + 5);
	log.SetBounds(layout.Width() * 4, 1000);
}


void ForgeScene::SetModel( bool randomTraits )
{
	const GameItem& humanMale = ItemDefDB::Instance()->Get( "humanMale" );
	techRequired = 0;
	int crystalArr[NUM_CRYSTAL_TYPES] = { 0 };
	crystalRequired.Set(0, crystalArr);

	int type	= ForgeScript::RING;
	int subType = 0;
	int partsFlags = 0;
	int effectFlags = 0;

	if ( itemType[ForgeScript::GUN].Down() )	type = ForgeScript::GUN;
	if ( itemType[ForgeScript::SHIELD].Down() )	type = ForgeScript::SHIELD;
	
	for( int i=0; i<ForgeScript::NUM_GUN_TYPES; ++i ) {
		gunType[i].SetVisible( type == ForgeScript::GUN );
	}
	for( int i=1; i<NUM_GUN_PARTS; ++i ) {
		gunParts[i].SetVisible( type == ForgeScript::GUN );
	}
	for( int i=1; i<NUM_RING_PARTS; ++i ) {
		ringParts[i].SetVisible( type == ForgeScript::RING );
	}

	if ( type == ForgeScript::GUN ) {
		if ( gunType[ForgeScript::BLASTER].Down() )			{ subType = ForgeScript::BLASTER; }
		else if ( gunType[ForgeScript::PULSE].Down() )		{ subType = ForgeScript::PULSE;	  }
		else if ( gunType[ForgeScript::BEAMGUN].Down() )	{ subType = ForgeScript::BEAMGUN; }

		if ( gunParts[GUN_CELL].Down() )					{ partsFlags |= WeaponGen::GUN_CELL; }
		if ( gunParts[GUN_DRIVER].Down() )					{ partsFlags |= WeaponGen::GUN_DRIVER; }
		if ( gunParts[GUN_SCOPE].Down() )					{ partsFlags |= WeaponGen::GUN_SCOPE; }
	}
	else if ( type == ForgeScript::RING ) {
		if ( ringParts[RING_GUARD].Down() )					{ partsFlags |= WeaponGen::RING_GUARD;	}
		if ( ringParts[RING_TRIAD].Down() )					{ partsFlags |= WeaponGen::RING_TRIAD;	}
		if ( ringParts[RING_BLADE].Down() )					{ partsFlags |= WeaponGen::RING_BLADE;	}
	}

	if ( effects[ForgeScript::EFFECT_FIRE].Down() )			effectFlags |= GameItem::EFFECT_FIRE;
	if ( effects[ForgeScript::EFFECT_SHOCK].Down() )		effectFlags |= GameItem::EFFECT_SHOCK;
	//if ( effects[ForgeScript::EFFECT_EXPLOSIVE].Down() )	effectFlags |= GameItem::EFFECT_EXPLOSIVE;

	const GameItem* mainItem = forgeData->itemComponent->GetItem();
	int seed = mainItem->ID() ^ mainItem->Traits().Experience();
	ForgeScript forgeScript(seed,
							mainItem->Traits().Level(),
							forgeData->tech);

	int team = forgeData->itemComponent->ParentChit() ? forgeData->itemComponent->ParentChit()->Team() : 0;

	GameItem* newItem = forgeScript.Build( type, subType, 
						partsFlags, effectFlags, 
						&crystalRequired, &techRequired, randomTraits, 0 );
	delete item;
	item = newItem;

	Chit* parentChit = forgeData->itemComponent->ParentChit();;
	LumosChitBag* chitBag = 0;
	if (parentChit) {
		chitBag = parentChit->Context()->chitBag;
	}
	itemDescWidget.SetInfo( item, &humanMale, false, chitBag );

	ProcRenderInfo info;
	if ( item ) {
		engine->FreeModel( model );
		model = engine->AllocModel( item->ResourceName() );
		model->SetPos( 0, 0, 0 );

		// Rotate the shield to face the camera.
		Quaternion q;
		static const Vector3F AXIS = { 0, 0, 1 };
		q.FromAxisAngle( AXIS, type == ForgeScript::SHIELD ? -90.0f : 0.0f );
		model->SetRotation( q );

		AssignProcedural(item, &info);

		model->SetTextureXForm( info.te.uvXForm.x, info.te.uvXForm.y, info.te.uvXForm.z, info.te.uvXForm.w );
		model->SetTextureClip( info.te.clip.x, info.te.clip.y, info.te.clip.z, info.te.clip.w );
		model->SetColorMap( info.color );
		model->SetBoneFilter( info.filterName, info.filter );
	}
	CStr<64> str;
	str.Format( "Tech: %d", techRequired );
	techRequiredLabel.SetText( str.c_str() );
	crystalRequiredWidget.Set( crystalRequired );
	
	if (   item 
		&& forgeData->itemComponent->CanAddToInventory()
		&& forgeData->itemComponent->GetItem()->wallet.CanWithdraw(crystalRequired)
		&& techRequired <= forgeData->tech ) 
	{ 
		buildButton.SetEnabled( true );
	}
	else {
		buildButton.SetEnabled( false );
	}
}


void ForgeScene::ItemTapped( const gamui::UIItem* uiItem )
{
	if ( uiItem == &okay ) {
		lumosGame->PopScene();
	}
	else if ( uiItem == &buildButton ) {
		// This can only be tapped if enabled. We don't need to check for enough
		// resources to build. Start with the SetModel() call, which will
		// apply the features and make the die roll.
		SetModel( true );

		int count = crystalRequired.NumCrystals();
		
		forgeData->itemComponent->AddCraftXP( count );
		forgeData->itemComponent->AddToInventory( item );
		forgeData->itemComponent->GetItem()->historyDB.Increment( "Crafted" );
		// becomes part of the item, and will be returned to Reserve when item is destroyed.
		item->wallet.Deposit(&forgeData->itemComponent->GetItem()->wallet, 0, crystalRequired.Crystal());

		Chit* chit = forgeData->itemComponent->ParentChit();
		NewsHistory* history = (chit && chit->Context()->chitBag) ? chit->Context()->chitBag->GetNewsHistory() :0;	// eek. hacky.
		if (chit && history) {
			Vector2F pos = ToWorld2F(chit->Position());
			item->SetSignificant(history, pos, NewsEvent::FORGED, NewsEvent::UN_FORGED, chit);
		}

		logText.AppendFormat("%s forged! Value=%d.\n", item->Name(), item->GetValue());
		log.SetText(logText.c_str());
		random.Rand();
		item = new GameItem();
	}
	crystalAvailWidget.Set(forgeData->itemComponent->GetItem(0)->wallet);
	SetModel(true);
}


grinliz::Color4F ForgeScene::ClearColor()
{
	Color4F c = game->GetPalette()->Get4F(0, 5);
	return c;
}


void ForgeScene::Draw3D( U32 deltaTime )
{
	// we use our own screenport
	screenport.SetPerspective();
	engine->Draw( deltaTime, 0, 0 );
	screenport.SetUI();
}




