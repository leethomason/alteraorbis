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

	testMap = new TestMap( 12, 12 );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), testMap );
	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );
	engine->CameraLookAt( 6, 6, 10 );

	Load();
}


ParticleScene::~ParticleScene()
{
	Clear();
	delete engine;
	delete testMap;
}


void ParticleScene::Clear()
{
	for( int i=0; i<buttonArr.Size(); ++i ) {
		delete buttonArr[i];
	}
	buttonArr.Clear();
}


void ParticleScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	int x=0;
	int y=0;
	for( int i=0; i<buttonArr.Size(); ++i ) {
		layout.PosAbs( buttonArr[i], x*2, y );
		++x;
		if ( x == 5 ) {
			++y; x = 0;
		}
	}
}


void ParticleScene::Load()
{
	Clear(); 

	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	LayoutCalculator layout = lumosGame->DefaultLayout();
	const ButtonLook& look = lumosGame->GetButtonLook( LumosGame::BUTTON_LOOK_STD );

	particleDefArr.Clear();
	LoadParticles( &particleDefArr, "./res/particles.xml" );

	for( int i=0; i<particleDefArr.Size(); ++i )
	{
		const ParticleDef& pd = particleDefArr[i];

		Button* button = 0;
		if ( pd.time == ParticleDef::CONTINUOUS ) {
			button = new ToggleButton( &gamui2D, look );
		}
		else {
			button = new PushButton( &gamui2D, look );
		}
		button->SetSize( layout.Width()*2, layout.Height() );
		button->SetText( pd.name.c_str() );
		buttonArr.Push( button );
	}
	Resize();
}


void ParticleScene::Rescan()
{
	XMLDocument doc;
	doc.LoadFile( "./res/particles.xml" );

	for( const XMLElement* partEle = XMLConstHandle( doc ).FirstChildElement( "particles" ).FirstChildElement( "particle" ).ToElement();
		 partEle;
		 partEle = partEle->NextSiblingElement( "particle" ) )
	{
		const char* name = partEle->Attribute( "name" );
		for( int i=0; i<particleDefArr.Size(); ++i ) {
			if ( particleDefArr[i].name == name ) {
				particleDefArr[i].Load( partEle );
			}
		}
	}
}


void ParticleScene::ItemTapped( const gamui::UIItem* item )
{
	GLASSERT( buttonArr.Size() == particleDefArr.Size() );
	if ( item == &okay ) {
		game->PopScene();
	}
	for( int i=0; i<buttonArr.Size(); ++i ) {
		if ( buttonArr[i] == item ) {
			ParticleDef* def = &particleDefArr[i];
			Rescan();

			if ( buttonArr[i]->ToPushButton() ) {
				Vector3F pos = { 6.f, 0.f, 6.f };
				Vector3F normal = { 0, 1, 0 };
				Vector3F dir = { 1, 0, 0 };
				if ( def->config == ParticleSystem::PARTICLE_WORLD ) {
					pos.y = 0.1f;
				}
				engine->particleSystem->EmitPD( *def, pos, normal, dir, engine->camera.EyeDir3(), 0 );
			}
		}
	}
}


void ParticleScene::DoTick( U32 deltaTime )
{
	for( int i=0; i<buttonArr.Size(); ++i ) {
		ToggleButton* toggle = buttonArr[i]->ToToggleButton();
		if ( toggle && toggle->Down() ) {
			Vector3F pos = { 6.f, 0.f, 6.f };
			Vector3F normal = { 0, 1, 0 };
			Vector3F dir = { 1, 0, 0 };
			ParticleDef* def = &particleDefArr[i];
			engine->particleSystem->EmitPD( *def, pos, normal, dir, engine->camera.EyeDir3(), deltaTime );
		}
	}
}


void ParticleScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}
