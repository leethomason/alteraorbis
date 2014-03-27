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

#include "rendertestscene.h"
#include "../game/lumosgame.h"
#include "../gamui/gamui.h"
#include "../engine/engine.h"
#include "../xegame/testmap.h"
#include "../grinliz/glcolor.h"
#include "../engine/text.h"
#include "../script/procedural.h"

using namespace gamui;
using namespace tinyxml2;
using namespace grinliz;

const static int SIZE = 64;

RenderTestScene::RenderTestScene( LumosGame* game, const RenderTestSceneData* data ) : Scene( game ), lumosGame( game )
{
	testMap = new TestMap( SIZE, SIZE );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), testMap );
	glowLayer = -1;

	for( int i=0; i<NUM_MODELS; ++i )
		model[i] = 0;

	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );

	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	
	switch( data->id ) {
	case 0:
		SetupTest0();
		break;
	case 1:
		SetupTest1();
		break;
	default:
		GLASSERT( 0 );
		break;
	};
	LayoutCalculator layout = lumosGame->DefaultLayout();

	lumosGame->InitStd( &gamui2D, &okay, 0 );

	refreshButton.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	refreshButton.SetText( "refresh" );

	for( int i=0; i<NUM_CONTROL; ++i ) {
		control[i].Init( &gamui2D, game->GetButtonLook( 0 ) );
		CStr<16> str;
		str.Format( "%.2f", (float)i/(float)(NUM_CONTROL-1) );
		control[i].SetText( str.c_str() );
		control[0].AddToToggleGroup( &control[i] );
	}

	RenderAtom nullAtom;
	rtImage.Init( &gamui2D, nullAtom, true );
	rtImage.SetSize( 400.f, 200.f );

	LoadLighting();
}


RenderTestScene::~RenderTestScene()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		engine->FreeModel( model[i] );
	}
	delete testMap;
	delete engine;
}


void RenderTestScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );
	
	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &refreshButton, 2, -1 );

	for( int i=0; i<NUM_CONTROL; ++i ) {
		control[i].SetVisible( game->GetDebugUI() );
		layout.PosAbs( &control[i], i, -2 );
	}
}



void RenderTestScene::SetupTest0()
{
	const ModelResource* res0 = ModelResourceManager::Instance()->GetModelResource( "humanFemale" );
	const ModelResource* res1 = ModelResourceManager::Instance()->GetModelResource( "humanMale" );

	for( int i=0; i<NUM_MODELS/2; ++i ) {
		model[i] = engine->AllocModel( i<3 ? res0 : res1 );
		model[i]->SetPos( 1, 0, (float)i );
		model[i]->SetYRotation( (float)(i*30) );
	}
	for( int i=NUM_MODELS/2; i<NUM_MODELS; ++i ) {
		model[i] = engine->AllocModel( (i-NUM_MODELS/2)<3 ? res0 : res1 );
		model[i]->SetPos( 2, 0, (float)(i-NUM_MODELS/2) );
		model[i]->SetYRotation( (float)(i*30) );
		model[i]->SetAnimation( ANIM_WALK, 1000, true ); 
	}

	for ( int i=0; i<NUM_MODELS; ++i ) {
		ProcRenderInfo info;
		AssignProcedural( "suit", model[i]->GetResource() == res0, i, 1, false, 0, 0, &info ); 
		model[i]->SetTextureXForm( info.te.uvXForm );
		model[i]->SetColorMap( info.color );
		model[i]->SetBoneFilter( info.filterName, info.filter );
	}
	engine->CameraLookAt( 2, (float)(NUM_MODELS/4), 12 );
}


void RenderTestScene::SetupTest1()
{
	const ModelResource* testStruct0Res = ModelResourceManager::Instance()->GetModelResource( "testStruct0" );
	const ModelResource* testStruct1Res = ModelResourceManager::Instance()->GetModelResource( "testStruct1" );

	model[0] = engine->AllocModel( testStruct0Res );
	model[0]->SetPos( 0.5f, 0.0f, 1.0f );

	model[1] = engine->AllocModel( testStruct1Res );
	model[1]->SetPos( 0.5f, 0.0f, 3.5f );

	engine->CameraLookAt( 0, 3, 8, -45.f, -30.f );
}


void RenderTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		Process3DTap( action, view, world, engine );
	}
}


void RenderTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &refreshButton ) {
		LoadLighting();
	}

	for( int i=0; i<NUM_CONTROL; ++i ) {
		if ( item == &control[i] ) {
			for( int k=0; k<NUM_MODELS; ++k ) {
				if ( model[k] ) {
					float v = (float)i/(float)(NUM_CONTROL-1);
					model[k]->SetFadeFX( v );
				}
			}
			break;
		}
	}
}


void RenderTestScene::LoadLighting()
{
	XMLDocument doc;
	doc.LoadFile( "./res/lighting.xml" );

	const XMLElement* lightingEle = doc.FirstChildElement( "lighting" );
	const XMLElement* mapEle = lightingEle->FirstChildElement( "map" );

	if ( mapEle ) {
		Color3F mapColor;
		LoadColor( mapEle, &mapColor );
		testMap->SetColor( mapColor );
	}

	engine->lighting.Load( lightingEle );
}


void RenderTestScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void RenderTestScene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void RenderTestScene::HandleHotKey( int mask )
{
	switch( mask ) {
	case GAME_HK_TOGGLE_UI:
		{
			bool visible = !okay.Visible();
			okay.SetVisible( visible );
			refreshButton.SetVisible( visible );
		}
		break;

	case GAME_HK_SPACE:
		glowLayer++;
		if ( glowLayer == Engine::RT_COUNT ) {
			glowLayer = -1;
		}
		break;
	}

	super::HandleHotKey( mask );
}


void RenderTestScene::Draw3D( U32 deltaTime )
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( model[i] ) {
			model[i]->DeltaAnimation( deltaTime, 0, 0 );
		}
	}
	engine->Draw( deltaTime );
	
	if ( glowLayer >= 0 ) {
		rtImage.SetVisible( true );
		const Screenport& port = engine->GetScreenport();

		RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, 
			(const void*)engine->GetRenderTargetTexture(glowLayer), 0, 0, 1, 1 );
		rtImage.SetAtom( atom );
		rtImage.SetSize( port.UIWidth(), port.UIHeight() );
		rtImage.SetPos( 0, 0 );
	}
	else {
		rtImage.SetVisible( false );
	}
}


void RenderTestScene::DrawDebugText()
{
	DrawDebugTextDrawCalls( 0, 16, engine );
}

