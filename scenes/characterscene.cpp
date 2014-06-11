#include "characterscene.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"

#include "../engine/engine.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"

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

	game->InitStd( &gamui2D, &okay, &cancel );
	dropButton.Init( &gamui2D, lumosGame->GetButtonLook(0));
	dropButton.SetText( "Drop" );
	dropButton.SetVisible( data->IsAvatar() );

	reset.Init( &gamui2D, lumosGame->GetButtonLook(0));
	reset.SetText( "Reset" );
	reset.SetVisible( data->IsMarket() );

	billOfSale.Init( &gamui2D );
	billOfSale.SetVisible( data->IsMarket() );

	faceWidget.Init( &gamui2D, lumosGame->GetButtonLook(0), 0 );
	const GameItem* mainItem = data->itemComponent->GetItem(0);
	faceWidget.SetFace( &uiRenderer, mainItem );
	if (mainItem->keyValues.GetIString(ISC::mob) != ISC::denizen) {
		faceWidget.SetVisible(false);
	}

	desc.Init(&gamui2D);
	
	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			itemButton[j][i].Init( &gamui2D, lumosGame->GetButtonLook(0) );

			Button* button = faceWidget.GetButton();
			if ( button->ToToggleButton() ) {
				button->ToToggleButton()->AddToToggleGroup( &itemButton[j][i] );
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
	if ( data->IsMarket() || data->IsExchange() ) {
		moneyWidget[1].SetVisible( true );
		moneyWidget[1].Set( data->storageIC->GetItem(0)->wallet );
	}
	CalcCrystalValue();

	engine->lighting.direction.Set( 0, 1, 1 );
	engine->lighting.direction.Normalize();
	
	engine->camera.SetPosWC( 2, 2, 2 );
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	SetButtonText();
}


CharacterScene::~CharacterScene()
{
	engine->FreeModel( model );
	delete engine;
}


void CharacterScene::Resize()
{
	// Dowside of a local Engine: need to resize it.
	const Screenport& port = game->GetScreenport();
	engine->GetScreenportMutable()->Resize( port.PhysicalWidth(), port.PhysicalHeight() );

	lumosGame->PositionStd( &okay, &cancel );

	LayoutCalculator layout = lumosGame->DefaultLayout();

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
	layout.PosAbs( &billOfSale, 1, 6 );	
	layout.PosAbs( &reset, -1, -2 );
	layout.PosAbs(&helpText, 1, -3);
	helpText.SetBounds(cancel.X() - (okay.X() + okay.Width() - layout.GutterX()), 0);
	if (data->IsAvatar()) {
		// Need space for the history text
		helpText.SetBounds(port.UIWidth()*0.5f - (okay.X() + okay.Width()), 0);
	}

	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		layout.PosAbs(&crystalButton[0][i], 1, 1 + i);
		layout.PosAbs(&crystalButton[1][i], -4, 1 + i);
	}
	cancel.SetVisible(!data->IsExchange());

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


void CharacterScene::CalcCrystalValue()
{
	ForgeScript script(0, 2, 0);
	GameItem* item = new GameItem();
	Wallet wallet;
	int tech = 0;
	script.Build(ForgeScript::GUN, ForgeScript::BLASTER, 0, 0, item, &wallet, &tech, false);
	crystalValue[0] = int(item->GetValue()+1.0f);
	delete item;

	crystalValue[CRYSTAL_RED]		= crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_RED;
	crystalValue[CRYSTAL_BLUE]		= crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_BLUE;
	crystalValue[CRYSTAL_VIOLET]	= crystalValue[0] * ALL_CRYSTAL_GREEN / ALL_CRYSTAL_VIOLET;
}


grinliz::Color4F CharacterScene::ClearColor()
{
	Color4F c = game->GetPalette()->Get4F(0, 5);
	return c;
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
			str.Format("%s\n%d", NAMES[i], crystalValue[i]);
			crystalButton[j][i].SetText(str.c_str());
			crystalButton[j][i].SetDeco(atom, atom);
		}
	}
}


void CharacterScene::SetButtonText()
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

		const GameItem* mainItem		= ic->GetItem(0);
		const IRangedWeaponItem* ranged = ic->GetRangedWeapon(0);
		const IMeleeWeaponItem*  melee  = ic->GetMeleeWeapon();
		const IShield*           shield = ic->GetShield();
		const GameItem* rangedItem		= ranged ? ranged->GetItem() : 0;
		const GameItem* meleeItem		= melee  ? melee->GetItem() : 0;
		const GameItem* shieldItem		= shield ? shield->GetItem() : 0;
		float costMult = 0;
		if ( data->IsMarket() ) {
			costMult = 1.0f + SALES_TAX;
		}

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
			
		engine->FreeModel( model );
		model = 0;
		if ( !model ) {
			model = engine->AllocModel( down->ResourceName() );
			model->SetPos( 0,0,0 );
			if (down->GetItem()->IName() == ISC::shield) {
				Quaternion q;
				Matrix4 m;
				m.ConcatRotation(90.0f, 0);
				//m.ConcatRotation(90.0f, 1);
				q.FromRotationMatrix(m);
				model->SetRotation(q);
			}
			else if (down->GetItem()->keyValues.GetIString(ISC::mob) == IString()) {
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
			Vector3F lookAt = aabb.Center();
			lookAt.y = y;
			engine->CameraLookAt( engine->camera.PosWC(), aabb.Center() );

			IString proc = down->keyValues.GetIString( "procedural" );
			if ( !proc.empty() ) {
				ProcRenderInfo info;
				int features = 0;
				down->keyValues.Get( ISC::features, &features );

				AssignProcedural(	proc.c_str(), 
									strstr( down->Name(), "emale" )!=0, 
									down->ID(), 
									down->team, 
									false, 
									down->flags & GameItem::EFFECT_MASK, 
									features,
									&info );
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

	int bought=0, sold=0;
	CalcCost( &bought, &sold );

	if ( data->IsMarket() ) {
		CStr<100> str;
		str.Format( "Buy: %d\n"
					"Sell: %d\n"
					"%s: %d\n", bought, sold, (sold > bought) ? "Earn" : "Cost", abs(bought-sold) );

		int bought=0, sold=0;
		CalcCost( &bought, &sold);
		int cost = bought - sold;

		if ( bought > sold ) {
			const Wallet& wallet = data->itemComponent->GetItem()->wallet;
			bool enoughInWallet = cost <= wallet.gold;
			okay.SetEnabled( enoughInWallet );
			if ( !enoughInWallet ) {
				str.AppendFormat( "%s", "Not enough Au in wallet." );
			}
		}
		else {
			const Wallet& wallet = data->storageIC->GetItem()->wallet;
			bool enoughInMarket = -cost <= wallet.gold;
			okay.SetEnabled( enoughInMarket );
			if ( !enoughInMarket ) {
				str.AppendFormat( "%s", "Market has insufficient Au." );
			}
		}
		billOfSale.SetText( str.c_str() );
	}
}


void CharacterScene::ResetInventory()
{
	CDynArray< GameItem* > player, store; 

	// Remove everything so we don't overflow
	// an inventory. And then add stuff back.
	for( int i=0; i<boughtList.Size(); ++i ) {
		int index = data->itemComponent->FindItem( boughtList[i] );
		GLASSERT( index >= 0 );
		store.Push( data->itemComponent->RemoveFromInventory( index ));
	}
	for( int i=0; i<soldList.Size(); ++i ) {
		int index = data->storageIC->FindItem( soldList[i] );
		GLASSERT( index >= 0 );
		player.Push( data->storageIC->RemoveFromInventory( index ));
	}

	for( int i=0; i<player.Size(); ++i ) {
		data->itemComponent->AddToInventory( player[i] );
	}
	for( int i=0; i<store.Size(); ++i ) {
		data->storageIC->AddToInventory( store[i] );
	}
	boughtList.Clear();
	soldList.Clear();
}


void CharacterScene::Activate()
{
	if ((data->IsExchange() || data->IsMarket()) && ReserveBank::Instance()) {
		// The exchange works with the ReserveBank, else it
		// would be running out of money, and have a fixed
		// limit to the crystal/Au transaction.
		int d = 1000 - data->storageIC->GetItem()->wallet.gold;
		d = Min(d, ReserveBank::Instance()->bank.gold);
		Transfer(&data->storageIC->GetItem()->wallet, &ReserveBank::Instance()->bank, d);
		moneyWidget[1].Set(data->storageIC->GetItem()->wallet);
	}
}


void CharacterScene::DeActivate()
{
	if ((data->IsExchange() || data->IsMarket()) && ReserveBank::Instance()) {
		// Move all the gold to the reserve
		Transfer(&ReserveBank::Instance()->bank, &data->storageIC->GetItem()->wallet, data->storageIC->GetItem()->wallet.gold);
	}
}


void CharacterScene::ItemTapped(const gamui::UIItem* item)
{
	if ( item == &okay ) {
		if ( data->IsMarket() ) {
			int bought=0, sold=0, salesTax=0;
			CalcCost( &bought, &sold);

			int cost = bought - sold;
			data->itemComponent->GetItem(0)->wallet.AddGold( -cost );
			data->storageIC->GetItem(0)->wallet.AddGold( cost );

			if (data->taxRecipiant) {
				GLASSERT(data->storageIC->GetItem(0)->wallet.gold > salesTax);
				Transfer(data->taxRecipiant, &data->storageIC->GetItem(0)->wallet, salesTax);
			}
		}
		lumosGame->PopScene();
		billOfSale.SetVisible(false);	// so errant warning can't be seen.
	}
	if ( item == &cancel ) {
		ResetInventory();
		lumosGame->PopScene();
	}
	if ( item == &reset ) {
		ResetInventory();
	}

	for (int origin = 0; origin < 2; ++origin) {
		for (int crystal = 0; crystal < NUM_CRYSTAL_TYPES; ++crystal) {
			if (item == &crystalButton[origin][crystal]) {
				Wallet transferCrystal;
				transferCrystal.crystal[crystal] = 1;
				Wallet transferGold;
				transferGold.gold = crystalValue[crystal];

				Wallet* avatarWallet = &data->itemComponent->GetItem()->wallet;
				Wallet* exchangeWallet = &data->storageIC->GetItem()->wallet;

				if (origin == 0) {
					if (transferCrystal <= *avatarWallet && transferGold <= *exchangeWallet) {
						Transfer(exchangeWallet, avatarWallet, transferCrystal);
						Transfer(avatarWallet, exchangeWallet, transferGold);
					}
				}
				else {
					if (transferCrystal <= *exchangeWallet && transferGold <= *avatarWallet) {
						Transfer(avatarWallet, exchangeWallet, transferCrystal);
						Transfer(exchangeWallet, avatarWallet, transferGold);
					}
				}
				moneyWidget[0].Set(data->itemComponent->GetItem()->wallet);
				moneyWidget[1].Set(data->storageIC->GetItem()->wallet);
				break;
			}
		}
	}

	SetButtonText();
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
	int startIndex = 0;
	ItemComponent* startIC = 0;
	int endIndex   = 0;
	ItemComponent* endIC = 0;

	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( start == &itemButton[j][i] ) {
				startIndex = itemButtonIndex[j][i];
				startIC = (j==0) ? data->itemComponent : data->storageIC;
				break;
			}
		}
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( end == &itemButton[j][i] ) {
				endIndex = itemButtonIndex[j][i];
				endIC = (j==0) ? data->itemComponent : data->storageIC;
				break;
			}
		}
	}
	if ( startIndex && endIndex ) {
		ItemComponent::Swap2( startIC, startIndex, endIC, endIndex );
	}
	else if ( startIndex && endIC ) {
		GameItem* item = startIC->RemoveFromInventory( startIndex );
		if ( item ) {
			endIC->AddToInventory( item );

			if ( endIC != startIC ) {
				// Moved from one inventory to the other.
				// Buying or selling?
				if ( startIC == data->itemComponent ) {
					// we are selling:
					int i = boughtList.Find( item );
					if ( i >= 0 ) {
						// changed our minds - return to store.
						boughtList.SwapRemove( i );
					}
					else {
						GLASSERT( soldList.Find( item ) < 0 );
						soldList.Push( item );
					}
				}
				else {
					// we are buying:
					int i = soldList.Find( item );
					if ( i >= 0 ) {
						// changed our minds - back to inventory.
						soldList.SwapRemove( i );
					}
					else {
						GLASSERT( boughtList.Find( item ) < 0 );
						boughtList.Push( item );
					}
				}
			}
		}
	}


	if ( data->IsAvatar() && start && startIndex && end == &dropButton ) {
		data->itemComponent->Drop( data->itemComponent->GetItem( startIndex ));
	}

	SetButtonText();
}


void CharacterScene::CalcCost( int* bought, int* sold )
{
	*bought = 0;
	*sold = 0;

	for( int i=0; i<boughtList.Size(); ++i ) {
		int value = boughtList[i]->GetValue();
		*bought += value;
	}
	for( int i=0; i<soldList.Size(); ++i ) {
		int value = soldList[i]->GetValue();
		*sold += value;
	}
}
