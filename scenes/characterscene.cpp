#include "characterscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../xegame/itemcomponent.h"
#include "../script/procedural.h"
#include "../script/battlemechanics.h"

using namespace gamui;
using namespace grinliz;

static const float NEAR = 0.1f;
static const float FAR  = 10.0f;

CharacterScene::CharacterScene( LumosGame* game, CharacterSceneData* csd ) : Scene( game ), screenport( game->GetScreenport() )
{
	this->lumosGame = game;
	this->itemComponent = csd->itemComponent;
	vault = csd->vault;
	nStorage = vault ? 2 : 1;
	model = 0;

	screenport.SetNearFar( NEAR, FAR );
	engine = new Engine( &screenport, lumosGame->GetDatabase(), 0 );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	game->InitStd( &gamui2D, &okay, 0 );
	dropButton.Init( &gamui2D, lumosGame->GetButtonLook(0));
	dropButton.SetText( "Drop" );

	faceWidget.Init( &gamui2D, lumosGame->GetButtonLook(0));
	faceWidget.SetFace( &uiRenderer, itemComponent->GetItem(0) );

	desc.Init( &gamui2D );
	
	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			itemButton[j][i].Init( &gamui2D, lumosGame->GetButtonLook(0) );
			faceWidget.GetToggleButton()->AddToToggleGroup( &itemButton[j][i] );
		}
	}
	itemDescWidget.Init( &gamui2D );
	moneyWidget.Init( &gamui2D );
	moneyWidget.Set( itemComponent->GetItem(0)->wallet );

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

	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();

	layout.PosAbs( &faceWidget, 0, 0 );
	faceWidget.SetSize( faceWidget.Width(), faceWidget.Height()*2.0f );

	layout.PosAbs( &moneyWidget, 1, 0, false );
	
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
	layout.PosAbs( &dropButton, 1, 7 );

	layout.PosAbs( &desc, -4, 0 );
	desc.SetSize( layout.Width() * 4.0f, layout.Height() );

	float y = desc.Y() + desc.Height();
	if ( vault ) {
		desc.SetPos( port.UIWidth()*0.5f - layout.Width()*0.5f, desc.Y() );
		y = dropButton.Y();
	}

	float x = desc.X();

	itemDescWidget.SetLayout( layout );
	itemDescWidget.SetPos( x, y );

	/*
	float dx = desc.Width() * 0.5f;

	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		textKey[i].SetPos( x, y );
		textVal[i].SetPos( x+dx, y );
		y += gamui2D.GetTextHeight() * 1.5f;
	}
	*/
}


void CharacterScene::SetItemInfo( const GameItem* item, const GameItem* user )
{
	if ( !item )
		return;

	CStr< 128 > str;

	str.Format( "%s\nLevel: %d  XP: %d / %d", item->ProperName() ? item->ProperName() : item->Name(), 
										 item->traits.Level(),
										 item->traits.Experience(),
										 GameTrait::LevelToExperience( item->traits.Level()+1 ));
	desc.SetText( str.c_str() );

	itemDescWidget.SetInfo( item, user );
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

		ItemComponent* ic = (j==0) ? itemComponent : vault;

		const GameItem* mainItem		= ic->GetItem(0);
		const IRangedWeaponItem* ranged = ic->GetRangedWeapon(0);
		const IMeleeWeaponItem*  melee  = ic->GetMeleeWeapon();
		const IShield*           shield = ic->GetShield();
		const GameItem* rangedItem		= ranged ? ranged->GetItem() : 0;
		const GameItem* meleeItem		= melee  ? melee->GetItem() : 0;
		const GameItem* shieldItem		= shield ? shield->GetItem() : 0;

		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			const GameItem* item = ic->GetItem(src);
			if ( !item || item->Intrinsic() ) {
				++src;
				continue;
			}

			// Set the text to the proper name, if we have it.
			// Then an icon for what it is, and a check
			// mark if the object is in use.
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
		down = itemComponent->GetItem(0);
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
				AssignProcedural(	proc.c_str(), 
									strstr( down->Name(), "emale" )!=0, 
									down->id, 
									down->primaryTeam, 
									false, 
									down->flags & GameItem::EFFECT_MASK, 
									0,
									&info );
				model->SetTextureXForm( info.te.uvXForm );
				model->SetColorMap( true, info.color );
				model->SetBoneFilter( info.filterName, info.filter );
			}
		}
	}

	if ( down != itemComponent->GetItem(0)) {
		SetItemInfo( down, itemComponent->GetItem(0) );
	}
	else {
		SetItemInfo( down, 0 );
	}
}


void CharacterScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
	for( int j=0; j<nStorage; ++j ) {
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( item == &itemButton[j][i] ) {
				SetButtonText();
			}
		}
	}
	if ( item == faceWidget.GetButton() ) {
		SetButtonText();
	}
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
				startIC = (j==0) ? itemComponent : vault;
				break;
			}
		}
		for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
			if ( end == &itemButton[j][i] ) {
				endIndex = itemButtonIndex[j][i];
				endIC = (j==0) ? itemComponent : vault;
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
		}
	}


	if ( !vault && start && startIndex && end == &dropButton ) {
		itemComponent->Drop( itemComponent->GetItem( startIndex ));
	}

	SetButtonText();
}
