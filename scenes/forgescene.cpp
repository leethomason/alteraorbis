#include "forgescene.h"
#include "../game/lumosgame.h"

using namespace gamui;
using namespace grinliz;

ForgeScene::ForgeScene( LumosGame* game, ForgeSceneData* data ) : Scene( game ), lumosGame( game )
{
	lumosGame->InitStd( &gamui2D, &okay, 0 );

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
}


ForgeScene::~ForgeScene()
{
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
}


void ForgeScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}


void ForgeScene::Draw3D( U32 delatTime )
{

}




