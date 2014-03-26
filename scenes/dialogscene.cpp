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

#include "dialogscene.h"

#include "../engine/uirendering.h"
#include "../engine/texture.h"
#include "../xegame/itemcomponent.h"
#include "../game/lumosgame.h"
#include "../script/itemscript.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

using namespace gamui;

DialogScene::DialogScene( LumosGame* game ) : Scene( game ), lumosGame( game ) 
{
	lumosGame->InitStd( &gamui2D, &okay, 0 );

	const ButtonLook& stdBL = lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	for ( int i=0; i<NUM_ITEMS; ++i ) {
		itemArr[i].Init( &gamui2D, stdBL );
	}
	itemArr[0].SetDeco( LumosGame::CalcUIIconAtom( "okay", true ), 
						LumosGame::CalcUIIconAtom( "okay", false ) );

	for( int i=0; i<NUM_TOGGLES; ++i ) {
		toggles[i].Init( &gamui2D, lumosGame->GetButtonLook(0));
		toggles[0].AddToToggleGroup( &toggles[i] );

		for( int j=0; j<NUM_SUB; ++j ) {
			int index = i*NUM_SUB+j;
			subButtons[index].Init( &gamui2D, lumosGame->GetButtonLook(0));
			toggles[i].AddSubItem( &subButtons[index] );
			subButtons[i*NUM_SUB].AddToToggleGroup( &subButtons[index] );
		}
	}

	for( int i=0; i<NUM_SCENES; ++i ) {
		static const char* name[NUM_SCENES] = { "character", "vault", "forge", "market" };
		sceneButtons[i].Init( &gamui2D, lumosGame->GetButtonLook(0));
		sceneButtons[i].SetText( name[i] );
	}

	ItemDefDB* db = ItemDefDB::Instance();
	const GameItem& human	= db->Get( "humanMale" );
	const GameItem& troll	= db->Get( "troll" );
	const GameItem& blaster	= db->Get( "blaster" );
	const GameItem& pistol	= db->Get( "pistol" );
	const GameItem& ring	= db->Get( "ring" );
	const GameItem& market	= db->Get( "market" );

	itemComponent0 = new ItemComponent( new GameItem( human ));
	itemComponent0->AddToInventory( new GameItem( blaster ));
	itemComponent0->AddToInventory( new GameItem( pistol ));
	itemComponent0->AddToInventory( new GameItem( ring ));

	itemComponent1 = new ItemComponent( new GameItem( troll ));
	itemComponent1->AddToInventory( new GameItem( blaster ));
	itemComponent1->AddToInventory( new GameItem( pistol ));
	itemComponent1->AddToInventory( new GameItem( ring ));
	itemComponent1->AddToInventory( new GameItem( ring ));
	itemComponent1->GetItem()->wallet.AddGold( 200 );

	marketComponent = new ItemComponent( new GameItem( market ));
	marketComponent->GetItem(0)->wallet.AddGold( 100 );
	marketComponent->AddToInventory( new GameItem( blaster ));
	marketComponent->AddToInventory( new GameItem( blaster ));
	marketComponent->AddToInventory( new GameItem( pistol ));

	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		itemComponent0->GetItem(0)->wallet.AddCrystal( i );
	}
	itemComponent0->GetItem()->GetTraitsMutable()->Roll( 10 );
	itemComponent0->GetItem()->GetPersonalityMutable()->Roll( 20, &itemComponent0->GetItem()->Traits() );
}

DialogScene::~DialogScene()
{
	itemComponent0->GetItem()->wallet.EmptyWallet();
	itemComponent1->GetItem()->wallet.EmptyWallet();
	marketComponent->GetItem()->wallet.EmptyWallet();

	delete itemComponent0;
	delete itemComponent1;
	delete marketComponent;
}


void DialogScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();

	for ( int i=0; i<NUM_ITEMS; ++i ) {
		layout.PosAbs( &itemArr[i], i, 0 );
	}

	for( int i=0; i<NUM_TOGGLES; ++i ) {
		layout.PosAbs( &toggles[i], 0, i+2 );

		for( int j=0; j<NUM_SUB; ++j ) {
			int index = i*NUM_SUB+j;
			layout.PosAbs( &subButtons[index], j+1, i+2 );
		}
	}

	for( int i=0; i<NUM_SCENES; ++i ) {
		layout.PosAbs( &sceneButtons[i], i, 5 );
	}
}


void DialogScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &sceneButtons[CHARACTER] ) {
		game->PushScene( LumosGame::SCENE_CHARACTER, new CharacterSceneData( itemComponent0, 0, 0 ));
	}
	else if ( item == &sceneButtons[VAULT] ) {
		game->PushScene( LumosGame::SCENE_CHARACTER, new CharacterSceneData( itemComponent0, itemComponent1, 0 ));
	}
	else if ( item == &sceneButtons[MARKET] ) {
		game->PushScene( LumosGame::SCENE_CHARACTER, new CharacterSceneData( itemComponent0, marketComponent, true ));
	}
	else if ( item == &sceneButtons[FORGE] ) {
		ForgeSceneData* data = new ForgeSceneData();
		data->itemComponent = itemComponent0;
		data->tech = 3;
		game->PushScene( LumosGame::SCENE_FORGE, data );
	}
}


void DialogScene::SceneResult( int sceneID, int result, const SceneData* data )
{
}


gamui::RenderAtom DialogScene::DragStart( const gamui::UIItem* item )
{
	Button* button = (Button*)item;
	RenderAtom atom, nullAtom;
	button->GetDeco( &atom, 0 );
	if ( !atom.Equal( nullAtom ) ) {
		button->SetDeco( nullAtom, nullAtom );
	}
	return atom;
}


void DialogScene::DragEnd( const gamui::UIItem* start, const gamui::UIItem* end )
{
	if ( end ) {
		((PushButton*)end)->SetDeco( LumosGame::CalcUIIconAtom( "okay", true ), 
			                         LumosGame::CalcUIIconAtom( "okay", false ) );
	}
	else if ( start ) {
		((PushButton*)start)->SetDeco( LumosGame::CalcUIIconAtom( "okay", true ), 
			                           LumosGame::CalcUIIconAtom( "okay", false ) );
	}
}
