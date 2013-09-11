#include "characterscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"

CharacterScene::CharacterScene( LumosGame* game ) : Scene( game )
{
	this->lumosGame = game;

	Screenport* port = game->GetScreenportMutable();
	engine = new Engine( port, lumosGame->GetDatabase(), 0 );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

	game->InitStd( &gamui2D, &okay, 0 );
}


CharacterScene::~CharacterScene()
{
	delete engine;
}


void CharacterScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
}


void CharacterScene::ItemTapped( const gamui::UIItem* item )
{
}

void CharacterScene::DoTick( U32 deltaTime )
{
}
	
void CharacterScene::Draw3D( U32 delatTime )
{
}

