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
	dropButton.SetVisible( !data->IsMarket() );

	reset.Init( &gamui2D, lumosGame->GetButtonLook(0));
	reset.SetText( "Reset" );
	reset.SetVisible( data->IsMarket() );

	billOfSale.Init( &gamui2D );
	billOfSale.SetVisible( data->IsMarket() );

	faceWidget.Init( &gamui2D, lumosGame->GetButtonLook(0), 0 );
	faceWidget.SetFace( &uiRenderer, data->itemComponent->GetItem(0) );

	desc.Init( &gamui2D );
	
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

	moneyWidget[0].Init( &gamui2D );
	moneyWidget[1].Init( &gamui2D );
	moneyWidget[1].SetVisible( false );

	moneyWidget[0].Set( data->itemComponent->GetItem(0)->wallet );
	if ( data->IsMarket() ) {
		moneyWidget[1].SetVisible( true );
		moneyWidget[1].Set( data->storageIC->GetItem(0)->wallet );
	}

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

	layout.PosAbs( &faceWidget, 0, 0 );
	faceWidget.SetSize( faceWidget.Width(), faceWidget.Height()*2.0f );
	
	for( int j=0; j<nStorage; ++j ) {
		int col=0;
		int row=0;

		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( j==0 )
				layout.PosAbs( &itemButton[j][i], col+1, row+1 );
			else
				layout.PosAbs( &itemButton[j][i], -3+col, row+1 );
			++col;
			if ( col == 3 ) {
				++row;
				col = 0;
			}
		}
	}
	layout.PosAbs( &moneyWidget[0], 1, 0, false );
	layout.PosAbs( &moneyWidget[1], -3, 0, false );
	layout.PosAbs( &dropButton, 1, 7 );
	layout.PosAbs( &billOfSale, 1, 7 );	
	layout.PosAbs( &reset, -1, -2 );

	layout.PosAbs( &desc, -4, 0 );
	desc.SetBounds( layout.Width() * 4.0f, 0 );

	float y = desc.Y() + desc.Height();
	if ( !data->IsCharacter() ) {
		desc.SetPos( port.UIWidth()*0.5f - layout.Width()*0.5f, desc.Y() );
		y = dropButton.Y();
	}

	float x = desc.X();

	itemDescWidget.SetLayout( layout );
	itemDescWidget.SetPos( x, y );
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

	ChitBag* chitBag = data->itemComponent->ParentChit()->GetChitBag();
	itemDescWidget.SetInfo( item, user, nStorage==1, chitBag );
}


void CharacterScene::SetButtonText()
{
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
			costMult = MARKET_COST_MULT;
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
				lumosGame->ItemToButton( item, &itemButton[j][count], j == 0 ? costMult : 1.0f / costMult );
			else
				lumosGame->ItemToButton( item, &itemButton[j][count], 0 );
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

			Rectangle3F aabb = model->AABB();
			float size = Max( aabb.SizeX(), Max( aabb.SizeY(), aabb.SizeZ() ));
			float d = 1.5f + size;
			engine->camera.SetPosWC( d, d, d );
			engine->CameraLookAt( engine->camera.PosWC(), aabb.Center() );

			IString proc = down->keyValues.GetIString( "procedural" );
			if ( !proc.empty() ) {
				ProcRenderInfo info;
				int features = 0;
				down->keyValues.Get( "features", &features );

				AssignProcedural(	proc.c_str(), 
									strstr( down->Name(), "emale" )!=0, 
									down->ID(), 
									down->primaryTeam, 
									false, 
									down->flags & GameItem::EFFECT_MASK, 
									features,
									&info );
				model->SetTextureXForm( info.te.uvXForm );
				model->SetColorMap( true, info.color );
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
		CalcCost( &bought, &sold );
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


void CharacterScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		if ( data->IsMarket() ) {
			int bought=0, sold=0;
			CalcCost( &bought, &sold );

			int cost = bought - sold;
			data->itemComponent->GetItem(0)->wallet.AddGold( -cost );
			data->storageIC->GetItem(0)->wallet.AddGold( cost );
		}
		lumosGame->PopScene();
	}
	if ( item == &cancel ) {
		ResetInventory();
		lumosGame->PopScene();
	}
	if ( item == &reset ) {
		ResetInventory();
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


	if ( data->IsCharacter() && start && startIndex && end == &dropButton ) {
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
		*bought += int( float(value) / MARKET_COST_MULT );
	}
	for( int i=0; i<soldList.Size(); ++i ) {
		int value = soldList[i]->GetValue();
		*sold += int( float(value) * MARKET_COST_MULT );
	}
}
