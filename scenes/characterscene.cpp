#include "characterscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../xegame/itemcomponent.h"
#include "../script/procedural.h"

using namespace gamui;
using namespace grinliz;

CharacterScene::CharacterScene( LumosGame* game, CharacterSceneData* csd ) : Scene( game )
{
	this->lumosGame = game;
	this->itemComponent = csd->itemComponent;
	model = 0;

	Screenport* port = game->GetScreenportMutable();
	engine = new Engine( port, lumosGame->GetDatabase(), 0 );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );



	game->InitStd( &gamui2D, &okay, 0 );

	for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
		itemButton[i].Init( &gamui2D, lumosGame->GetButtonLook(0) );
		itemButton[0].AddToToggleGroup( &itemButton[i] );
	}
	SetButtonText();

	engine->lighting.direction.Set( 0, 1, 1 );
	engine->lighting.direction.Normalize();
	
	engine->camera.SetPosWC( 2, 2, 2 );
	static const Vector3F out = { 1, 0, 0 };
	engine->camera.SetDir( out, V3F_UP );

	itemButton[0].SetDown();
	SetButtonText();
}


CharacterScene::~CharacterScene()
{
	engine->FreeModel( model );
	delete engine;
}


void CharacterScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &itemButton[0], 0, 0 );

	int col=0;
	int row=0;
	for( int i=1; i<NUM_ITEM_BUTTONS; ++i ) {
		layout.PosAbs( &itemButton[i], col, row+1 );
		++col;
		if ( col == 3 ) {
			++row;
			col = 0;
		}
	}
}


void CharacterScene::SetButtonText()
{
	int count=0;
	const GameItem* down = 0;
	for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
		const GameItem* item = itemComponent->GetItem(i);
		if ( item && !item->Intrinsic() ) {
			itemButton[count].SetText( item->name.c_str() );
			if ( itemButton[count].Down() ) {
				down = item;
			}
			++count;
		}
	}

	if ( down ) {
		if ( model && !StrEqual( model->GetResource()->Name(), down->ResourceName() )) {
			engine->FreeModel( model );
			model = 0;
		}
		if ( !model ) {
			model = engine->AllocModel( down->ResourceName() );
			model->SetPos( 0,0,0 );
			engine->CameraLookAt( engine->camera.PosWC(), model->AABB().Center() );

			if ( !down->GetValue( "procedural" ).empty() ) {
				ProcRenderInfo info;
				AssignProcedural( down->GetValue( "procedural" ).c_str(), strstr( down->Name(), "emale" )!=0, down->id, down->primaryTeam, false, down->flags & GameItem::EFFECT_MASK, &info );
				model->SetTextureXForm( info.te.uvXForm );
				model->SetColorMap( true, info.color );
				model->SetBoneFilter( info.filterName, info.filter );
			}
		}
	}
}


void CharacterScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
	for( int i=0; i<NUM_ITEM_BUTTONS; ++i ) {
		if ( item == &itemButton[i] ) {
			SetButtonText();
		}
	}
}

void CharacterScene::DoTick( U32 deltaTime )
{
}
	
void CharacterScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime, 0, 0 );
}

