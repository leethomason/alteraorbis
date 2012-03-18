#include "particlescene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../xegame/testmap.h"
#include "../tinyxml2/tinyxml2.h"

using namespace gamui;
using namespace grinliz;
using namespace tinyxml2;


ParticleScene::ParticleScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	testMap = new TestMap( 12, 12 );
	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );
	engine->SetMap( testMap );
	engine->CameraLookAt( 6, 6, 10 );

	Load();
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


void ParticleScene::Load()
{
	XMLDocument doc;
	doc.LoadFile( "particles.xml" );

	// FIXME: switch to safe version.
	for( const XMLElement* partEle = doc.FirstChildElement( "particles" )->FirstChildElement( "particle" );
		 partEle;
		 partEle = partEle->NextSiblingElement( "particle" ) )
	{
		GLOUTPUT(( "Particle: %s\n", partEle->Value() ));
	}
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
