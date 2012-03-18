#include "particlescene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../xegame/testmap.h"

using namespace gamui;
using namespace grinliz;

ParticleScene::ParticleScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	testMap = new TestMap( 12, 12 );
	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );
	engine->SetMap( testMap );
	engine->CameraLookAt( 6, 6, 10 );
}


ParticleScene::~ParticleScene()
{
	delete engine;
	delete testMap;
}


void ParticleScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );
}


void ParticleScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void ParticleScene::Draw3D()
{
	engine->Draw();
}
