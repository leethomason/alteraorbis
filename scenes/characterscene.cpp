#include "characterscene.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"

#include "../engine/engine.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitcontext.h"

#include "../script/procedural.h"
#include "../script/battlemechanics.h"
#include "../script/itemscript.h"
#include "../script/corescript.h"
#include "../script/forgescript.h"
#include "../ai/marketai.h"

#include "../game/reservebank.h"

using namespace gamui;
using namespace grinliz;

static const float NEAR = 0.1f;
static const float FAR  = 10.0f;

CharacterScene::CharacterScene( LumosGame* game, CharacterSceneData* csd ) : Scene( game ), screenport( game->GetScreenport() )
{
	this->lumosGame = game;
	data = csd;
	nStorage = data->storageIC ? 2 : 1;
	model = 0;

	screenport.SetNearFar( NEAR, FAR );
	engine = new Engine( &screenport, lumosGame->GetDatabase(), 0 );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	InitStd( &gamui2D, &okay, 0 );
	dropButton.Init( &gamui2D, lumosGame->GetButtonLook(0));
	dropButton.SetText( "Drop" );
	dropButton.SetVisible( data->IsAvatar() );

	faceWidget.Init( &gamui2D, lumosGame->GetButtonLook(0), 0, 0 );
	const GameItem* mainItem = data->itemComponent->GetItem(0);
	faceWidget.SetFace( &uiRenderer, mainItem );
	if (mainItem->keyValues.GetIString(ISC::mob).empty()) {
		faceWidget.SetVisible(false);
	}

	desc.Init(&gamui2D);
	
	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			itemButton[j][i].Init( &gamui2D, lumosGame->GetButtonLook(0) );

			Button* firstButton = faceWidget.Visible() ? faceWidget.GetButton() : &itemButton[0][0];
			if ( firstButton->ToToggleButton() ) {
				firstButton->ToToggleButton()->AddToToggleGroup( &itemButton[j][i] );
			}
		}
	}
	itemDescWidget.Init( &gamui2D );

	for (int j = 0; j < 2; ++j) {
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			crystalButton[j][i].Init(&gamui2D, game->GetButtonLook(0));
			crystalButton[j][i].SetVisible(false);
		}
	}

	moneyWidget[0].Init( &gamui2D );
	moneyWidget[1].Init( &gamui2D );
	moneyWidget[1].SetVisible( false );

	helpText.Init(&gamui2D);
	if (data->IsAvatar()) {
		helpText.SetText("Character inventory is on the left. You can order items by dragging. The gun, "
			"ring, and shield you want the character to use should be on the top row.");
	}
	else if (data->IsMarket()) {
		helpText.SetText("Character inventory is on the left. Market items are on the right. Dragging items from "
			"one inventory to the other will purchase or sell the item.");
		itemDescWidget.SetShortForm(true);
	}
	else if (data->IsVault()) {
		helpText.SetText("Avatar inventory is on the left. Vault contents are on the right. You can drag "
			"items between locations.");
		itemDescWidget.SetShortForm(true);
	}
	else if (data->IsExchange()) {
		helpText.SetText("Avatar Au and crystal is on the left. The exchange is on the right. Tap to buy or sell. "
			"The exchange is operater by the Reserve Bank and does not take a cut; you may trade freely.");
		itemDescWidget.SetShortForm(true);
	}

	moneyWidget[0].Set( data->itemComponent->GetItem(0)->wallet );
	if ( data->IsExchange() ) {
		moneyWidget[1].SetVisible( true );
		moneyWidget[1].Set( data->storageIC->GetItem(0)->wallet );
	}

	engine->lighting.direction.Set( 0, 1, 1 );
	engine->lighting.direction.Normalize();
	
	engine->camera.SetPosWC( 2, 2, 2 );
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	SetButtonText(csd->selectItem);
}


CharacterScene::~CharacterScene()
{
	delete model;
	delete engine;
}


void CharacterScene::Resize()
{
	// Dowside of a local Engine: need to resize it.
	const Screenport& port = game->GetScreenport();
	engine->GetScreenportMutable()->Resize( port.PhysicalWidth(), port.PhysicalHeight() );

	PositionStd( &okay, 0 );
	LayoutCalculator layout = DefaultLayout();

	layout.PosAbs( &faceWidget, 0, 1 );
	faceWidget.SetSize( faceWidget.Width(), faceWidget.Width() );
	
	for( int j=0; j<nStorage; ++j ) {
		int col=0;
		int row=0;

		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( j==0 )
				layout.PosAbs( &itemButton[j][i], col+1, row+1 );
			else
				layout.PosAbs( &itemButton[j][i], -4+col, row+1 );
			++col;
			if ( col == 3 ) {
				++row;
				col = 0;
			}
		}
	}
	layout.PosAbs( &moneyWidget[0], 1, 0, false );
	layout.PosAbs( &moneyWidget[1], -4, 0, false );
	layout.PosAbs( &dropButton, 1, 7 );
	layout.PosAbs(&helpText, 1, -3);
	if (data->IsAvatar()) {
		// Need space for the history text
		helpText.SetBounds(gamui2D.Width()*0.5f - (okay.X() + okay.Width()), 0);
	}

	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		layout.PosAbs(&crystalButton[0][i], 1, 1 + i);
		layout.PosAbs(&crystalButton[1][i], -4, 1 + i);
	}

	if (data->IsAvatarCharacterItem()) {
		layout.PosAbs(&desc, -4, 0);
		layout.PosAbs(&itemDescWidget, -4, 1);
	}
	else if (data->IsMarket()) {
		layout.PosAbs(&desc, -4, 6);
		layout.PosAbs(&itemDescWidget, -4, 7);
	}
	else if (data->IsVault()) {
		// Vault
		layout.PosAbs(&desc, -4, 0);
		layout.PosAbs(&itemDescWidget, -4, 6);
	}
	else if (data->IsExchange()) {
		desc.SetVisible(false);
		itemDescWidget.SetVisible(false);
	}
	float width = layout.Width() * 4;
	desc.SetBounds(width, 0);

	itemDescWidget.SetLayout(layout);
	itemDescWidget.SetPos(itemDescWidget.X(), itemDescWidget.Y());	// bug in the desc widgets
}


void CharacterScene::SetItemInfo( const GameItem* item, const GameItem* user )
{
	if ( !item )
		return;

	CStr< 128 > str;

	str.Format( "%s\nLevel: %d  XP: %d / %d", item->ProperName() ? item->ProperName() : item->Name(), 
										 item->Traits().Level(),
										 item->Traits().Experience(),
										 GameTrait::LevelToExperience( item->Traits().Level()+1 ));
	desc.SetText( str.c_str() );

	ChitBag* chitBag = data->itemComponent->ParentChit() ? data->itemComponent->ParentChit()->Context()->chitBag : 0;
	itemDescWidget.SetInfo( item, user, nStorage==1, chitBag );
}


void CharacterScene::SetExchangeButtonText()
{
	ReserveBank* bank = ReserveBank::Instance();
	GLASSERT(bank);

	for (int j = 0; j < 2; ++j) {
		for (int i = 0; i < NUM_ITEM_BUTTONS; ++i) {
			itemButton[j][i].SetVisible(false);
		}
	}
	for (int j = 0; j < 2; ++j) {
		static const char* NAMES[NUM_CRYSTAL_TYPES] = { "Green", "Red", "Blue", "Violet" };
		static const char* RES[NUM_CRYSTAL_TYPES] = { "crystalGreen", "crystalRed", "crystalBlue", "crystalViolet" };
		for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
			RenderAtom atom = LumosGame::CalcUIIconAtom(RES[i], true);
			crystalButton[j][i].SetVisible(true);

			CStr<32> str;
			str.Format("%s\n%d", NAMES[i], bank->CrystalValue(i));
			crystalButton[j][i].SetText(str.c_str());
			crystalButton[j][i].SetDeco(atom, atom);
		}
	}
}


void CharacterScene::SetButtonText(const GameItem* select)
{
	if (data->IsExchange()) {
		SetExchangeButtonText();
		return;
	}
	const GameItem* down = 0;

	RenderAtom nullAtom;
	RenderAtom iconAtom = LumosGame::CalcUIIconAtom( "okay" );
	for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
		itemButtonIndex[0][i] = 0;
		itemButtonIndex[1][i] = 0;
	}
	
	for( int j=0; j<nStorage; ++j ) {
		int count=0;
		int src = 1;

		ItemComponent* ic = (j==0) ? data->itemComponent : data->storageIC;

//		const GameItem* mainItem		= ic->GetItem(0);
		const RangedWeapon* rangedItem = ic->QuerySelectRanged();
		const MeleeWeapon* meleeItem = ic->QuerySelectMelee();
		const Shield* shieldItem = ic->GetShield();

		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			const GameItem* item = ic->GetItem(src);
			if ( !item || item->Intrinsic() ) {
				++src;
				continue;
			}

			// Set the text to the proper name, if we have it.
			// Then an icon for what it is, and a check
			// mark if the object is in use.
			if ( data->IsMarket() )
				lumosGame->ItemToButton( item, &itemButton[j][count] );
			else
				lumosGame->ItemToButton( item, &itemButton[j][count] );
			itemButtonIndex[j][count] = src;

			// Set the "active" icons.
			if ( item == rangedItem || item == meleeItem || item == shieldItem ) {
				itemButton[j][count].SetIcon( iconAtom, iconAtom );
			}
			else {
				itemButton[j][count].SetIcon( nullAtom, nullAtom );
			}

			if ( itemButton[j][count].Down() ) {
				down = item;
			}
			else if (select && item == select) {
				down = item;
				itemButton[j][count].SetDown();
			}
			++count;
			++src;
		}
		for( ; count<NUM_ITEM_BUTTONS; ++count ) {
			itemButton[j][count].SetText( " \n " );
			itemButton[j][count].SetIcon( nullAtom, nullAtom );
			itemButton[j][count].SetDeco( nullAtom, nullAtom );
		}
	}
	if ( !down ) {
		down = data->itemComponent->GetItem(0);
	}

	if ( down ) {
		// This doesn't work because we actually want to trigger off the item id.
		//if ( model && !StrEqual( model->GetResource()->Name(), down->ResourceName() )) {
			
		delete model;
		model = 0;
		if ( !model ) {
			model = new Model( down->ResourceName(), engine->GetSpaceTree() );
			model->SetPos( 0,0,0 );
			if (down->IName() == ISC::shield) {
				Quaternion q;
				Matrix4 m;
				m.ConcatRotation(90.0f, 0);
				//m.ConcatRotation(90.0f, 1);
				q.FromRotationMatrix(m);
				model->SetRotation(q);
			}
			else if (down->keyValues.GetIString(ISC::mob) == IString()) {
				model->SetYRotation(90.0f);
			}
			else {
				model->SetYRotation(0);
			}

			Rectangle3F aabb = model->AABB();
			float size = Max( aabb.SizeX(), Max( aabb.SizeY(), aabb.SizeZ() ));
			float d = 2.0f + size;
			float y = aabb.SizeY() * 0.7f;
			engine->camera.SetPosWC( 0, y, d );
//			Vector3F lookAt = aabb.Center();
//			lookAt.y = y;
			engine->CameraLookAt( engine->camera.PosWC(), aabb.Center() );

			IString proc = down->keyValues.GetIString( "procedural" );
			if ( !proc.empty() ) {
				ProcRenderInfo info;
				AssignProcedural(down, &info);

				model->SetTextureXForm( info.te.uvXForm );
				model->SetColorMap( info.color );
				model->SetBoneFilter( info.filterName, info.filter );
			}
		}
	}

	if ( down != data->itemComponent->GetItem(0)) {
		SetItemInfo( down, data->itemComponent->GetItem(0) );
	}
	else {
		SetItemInfo( down, 0 );
	}
}


void CharacterScene::Activate()
{
	if ((data->IsExchange() || data->IsMarket() ) && ReserveBank::Instance()) {
		// The exchange works with the ReserveBank, else it
		// would be running out of money, and have a fixed
		// limit to the crystal/Au transaction.
		int d = 1000;
		// The reserve bank has infinite funds, so we don't
		// need to range check.
		data->storageIC->GetItem()->wallet.Deposit(&ReserveBank::Instance()->wallet, d);
	}
}


void CharacterScene::DeActivate()
{
	if ((data->IsExchange() || data->IsMarket() ) && ReserveBank::Instance()) {
		// Move all the gold to the reserve
		// BUT NOT crystal. Crystal stays in the exchange.
		// Markets shouldn't collect crystal.
		Wallet* w = &data->storageIC->GetItem()->wallet;
		ReserveBank::Instance()->wallet.Deposit(w, w->Gold());
	}
}


void CharacterScene::ItemTapped(const gamui::UIItem* item)
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}

	for (int origin = 0; origin < 2; ++origin) {
		for (int crystal = 0; crystal < NUM_CRYSTAL_TYPES; ++crystal) {
			if (item == &crystalButton[origin][crystal]) {
				TransactAmt transferCrystal;
				transferCrystal.AddCrystal(crystal, 1);
				TransactAmt transferGold;
				transferGold.Set(ReserveBank::Instance()->CrystalValue(crystal), 0);

				Wallet* avatarWallet = &data->itemComponent->GetItem()->wallet;
				Wallet* exchangeWallet = &data->storageIC->GetItem()->wallet;

				if (origin == 0) {
					if (avatarWallet->CanWithdraw(transferCrystal) && exchangeWallet->CanWithdraw(transferGold)) {
						exchangeWallet->Deposit(avatarWallet, transferCrystal);
						avatarWallet->Deposit(exchangeWallet, transferGold);
					}
				}
				else {
					if (exchangeWallet->CanWithdraw(transferCrystal) && avatarWallet->CanWithdraw(transferGold)) {
						avatarWallet->Deposit(exchangeWallet, transferCrystal);
						exchangeWallet->Deposit(avatarWallet, transferGold);
					}
				}
				moneyWidget[0].Set(data->itemComponent->GetItem()->wallet);
				moneyWidget[1].Set(data->storageIC->GetItem()->wallet);
				break;
			}
		}
	}

	SetButtonText(0);
}

void CharacterScene::DoTick( U32 deltaTime )
{
}
	
void CharacterScene::Draw3D( U32 deltaTime )
{
	// we use our own screenport
	screenport.SetPerspective();
	engine->Draw( deltaTime, 0, 0 );
	screenport.SetUI();
}


gamui::RenderAtom CharacterScene::DragStart( const gamui::UIItem* item )
{
	RenderAtom atom, nullAtom;
	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( &itemButton[j][i] == item ) {

				itemButton[j][i].GetDeco( &atom, 0 );
				if ( !atom.Equal( nullAtom ) ) {
					itemButton[j][i].SetDeco( nullAtom, nullAtom );
				}
				return atom;
			}
		}
	}
	return nullAtom;
}


void CharacterScene::DragEnd( const gamui::UIItem* start, const gamui::UIItem* end )
{
	int startIndex = -1;
	int endIndex   = -1;
	ItemComponent* startIC = 0;
	ItemComponent* endIC = 0;

	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( start == &itemButton[j][i] ) {
				startIC = (j == 0) ? data->itemComponent : data->storageIC;
				startIndex = itemButtonIndex[j][i];
				break;
			}
		}
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( end == &itemButton[j][i] ) {
				endIC = (j == 0) ? data->itemComponent : data->storageIC;
				endIndex = itemButtonIndex[j][i];
				break;
			}
		}
	}

	if (startIC == endIC) {
		// Moving around in the same inventory.
		if ( startIndex && endIndex ) {
			ItemComponent::Swap2( startIC, startIndex, endIC, endIndex );
		}
	}
	else if (data->IsMarket()) {
		// Buy and Sell, don't swap
		if (startIC && endIC		// move between storage
			&& startIndex)			// is actually something selected in the start?
		{
			const GameItem* startItem = startIC->GetItem(startIndex);
			int value = startItem->GetValue();
			GLASSERT(value);

			bool canAfford = (endIC->GetItem()->wallet.Gold() >= value);

			if (value && canAfford && endIC->CanAddToInventory())  {
				GameItem* item = startIC->RemoveFromInventory(startItem);
				endIC->AddToInventory(item);
				startIC->GetItem()->wallet.Deposit(&endIC->GetItem()->wallet, value);
			}
		}
	}
	else if (data->IsVault()) {
		// Only swap.
		if (startIC && endIC && startIndex) {
			if (endIC->CanAddToInventory()) {
				const GameItem* startItem = startIC->GetItem(startIndex);
				GameItem* item = startIC->RemoveFromInventory(startItem);
				endIC->AddToInventory(item);
			}
		}
	}
	else if ( data->IsAvatar() ) {
		if (startIC && startIndex && (end == &dropButton)) {
			data->itemComponent->Drop(data->itemComponent->GetItem(startIndex));
		}
	}

	SetButtonText(0);

	if (data->itemComponent)
		moneyWidget[0].Set(data->itemComponent->GetItem()->wallet);
	if (data->storageIC)
		moneyWidget[1].Set(data->storageIC->GetItem()->wallet);
}
