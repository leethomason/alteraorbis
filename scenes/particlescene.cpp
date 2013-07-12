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

#include "particlescene.h"

#include "../engine/engine.h"

#include "../xegame/testmap.h"

#include "../tinyxml2/tinyxml2.h"

#include "../game/gamelimits.h"
#include "../game/layout.h"
#include "../game/lumosgame.h"

using namespace gamui;
using namespace grinliz;
using namespace tinyxml2;

static const int SIZE = 64;


ParticleScene::ParticleScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );

	testMap = new TestMap( SIZE, SIZE );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), testMap );
	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );
	engine->CameraLookAt( SIZE/2, SIZE/2, 10 );

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
	layout.SetSize( LAYOUT_SIZE_X*0.75f, LAYOUT_SIZE_Y*0.5f );
	int x=0;
	int y=1;
	for( int i=0; i<buttonArr.Size(); ++i ) {
		layout.PosAbs( buttonArr[i], x*2, y );
		++x;
		if ( x == 6 ) {
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
				Vector3F pos = { SIZE/2, 0.01f, SIZE/2 };
				Vector3F normal = { 0, 1, 0 };
				Vector3F dir = { 1, 0, 0 };
				engine->particleSystem->EmitPD( *def, pos, normal, 0 );
			}
		}
	}
}


void ParticleScene::DoTick( U32 deltaTime )
{
	for( int i=0; i<buttonArr.Size(); ++i ) {
		ToggleButton* toggle = buttonArr[i]->ToToggleButton();
		if ( toggle && toggle->Down() ) {
			Vector3F pos = { SIZE/2, 0.01f, SIZE/2 };
			Vector3F normal = { 0, 1, 0 };
			Vector3F dir = { 1, 0, 0 };
			ParticleDef* def = &particleDefArr[i];

			if ( def->spread == ParticleDef::SPREAD_POINT ) {
				engine->particleSystem->EmitPD( *def, pos, normal, deltaTime );
			}
			else {
				Rectangle3F r;
				r.Set( pos.x-0.5f, pos.y, pos.z-0.5f, pos.x+0.5f, pos.y, pos.z+0.5f ); 
				engine->particleSystem->EmitPD( *def, r, normal, deltaTime );
			}
		}
	}
}


void ParticleScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}



void ParticleScene::DrawDebugText()
{
	this->DrawDebugTextDrawCalls( 16, engine );
}


