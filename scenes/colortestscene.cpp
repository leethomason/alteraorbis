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

#include "colortestscene.h"
#include "../game/lumosgame.h"
#include "../gamui/gamui.h"
#include "../engine/engine.h"
#include "../xegame/testmap.h"
#include "../grinliz/glcolor.h"
#include "../engine/text.h"
#include "../script/procedural.h"
#include "../game/team.h"
#include "../game/gameitem.h"

using namespace gamui;
using namespace tinyxml2;
using namespace grinliz;

const static int SIZE = 64;

static const char* GUN_NAME[] = { "pistol", "blaster", "pulse", "beamgun" };


ColorTestScene::ColorTestScene( LumosGame* game) : Scene( game ), lumosGame( game )
{
	testMap = new TestMap( SIZE, SIZE );
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), testMap );

	Color3F c = { 0.5f, 0.5f, 0.5f };
	testMap->SetColor( c );

	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );
	engine->lighting.direction.Set(-2, 5, 1.5f);
	engine->lighting.direction.Normalize();
	
	SetupTest();
	LayoutCalculator layout = lumosGame->DefaultLayout();

	lumosGame->InitStd( &gamui2D, &okay, 0 );

	Texture* palette = TextureManager::Instance()->GetTexture("palette");
	RenderAtom atom((const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)palette, 0, 0, 1, 1);
	for (int i = 0; i < NUM_PAL_SEL; ++i) {
		paletteSelector[i].Init(&gamui2D, atom, true);
		paletteLabel[i].Init(&gamui2D);
		paletteVec[i].Set(i * 2, i + 1);
	}
	cameraHigh = true;
}


ColorTestScene::~ColorTestScene()
{
	for (int i = 0; i < NUM_MODELS; ++i) {
		engine->FreeModel(model[i]);
	}

	delete testMap;
	delete engine;
}


void ColorTestScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	for (int i = 0; i < NUM_PAL_SEL; ++i) {
		layout.PosAbs(&paletteSelector[i], i*2, 0, 2, 2);
		paletteSelector[i].SetCapturesTap(true);
		layout.PosAbs(&paletteLabel[i], i * 2, 2);
	}
	DoProcedural();
}



void ColorTestScene::SetupTest()
{
	model[HUMAN_MODEL] = engine->AllocModel("humanMale");
	model[HUMAN_MODEL]->SetPos(1, 0, 1);

	model[TEMPLE_MODEL] = engine->AllocModel("pyramid0");
	model[TEMPLE_MODEL]->SetPos(2.5f, 0, 1);

	model[SLEEPTUBE_MODEL] = engine->AllocModel("sleep");
	model[SLEEPTUBE_MODEL]->SetPos(4.0f, 0, 1);

	for (int i = RING_0_MODEL; i <= RING_1_MODEL; ++i) {
		model[i] = engine->AllocModel("ring");
		model[i]->SetPos(0.8f + 0.4f*float(i-RING_0_MODEL), 0.5f, 2.0f);
		model[i]->SetYRotation(90.0f);
	}

	for (int i = PISTOL_MODEL; i <= BEAMGUN_MODEL; ++i) {
		model[i] = engine->AllocModel(GUN_NAME[i - PISTOL_MODEL]);
		model[i]->SetPos(0.5f + 0.4f*float(i - PISTOL_MODEL), 0.5f, 2.8f);
		model[i]->SetYRotation(90.0f);
	}
	SetCamera();
}

void ColorTestScene::SetCamera()
{
	Vector3F cam    = { 1.0f, 5.0f, 8.0f };
	Vector3F target = { 2.5f, 0.5f, 1.0f };

	if (!cameraHigh) {
		cam.Set(1.0f, 1.0f, 6.0f);
		target.Set(0.8f, 0.5f, 1.0f);
	}

	engine->CameraLookAt(cam, target);
}


void ColorTestScene::DoProcedural()
{
	Vector4F buildBase = LumosGame::GetMainPalette()->Get4F(paletteVec[SEL_BUILDING_BASE].x, paletteVec[SEL_BUILDING_BASE].y);
	Vector4F buildContrast = LumosGame::GetMainPalette()->Get4F(paletteVec[SEL_BUILDING_CONTRAST].x, paletteVec[SEL_BUILDING_CONTRAST].y);
	Vector4F buildGlow = LumosGame::GetMainPalette()->Get4F(paletteVec[SEL_BUILDING_GLOW].x, paletteVec[SEL_BUILDING_GLOW].y);
	buildBase.a() = 0;
	buildContrast.a() = 0;
	buildGlow.a() = 0.7f;

	Vector4F base = LumosGame::GetMainPalette()->Get4F(paletteVec[SEL_BASE].x, paletteVec[SEL_BASE].y);
	Vector4F contrast = LumosGame::GetMainPalette()->Get4F(paletteVec[SEL_CONTRAST].x, paletteVec[SEL_CONTRAST].y);
	base.a() = 0;
	contrast.a() = 0;

	{
		ProcRenderInfo info;
		// Use to get the base xform, etc.
		AssignProcedural( "suit", false, 0, TEAM_HOUSE, false, 0, 0, &info ); 
		model[HUMAN_MODEL]->SetTextureXForm( info.te.uvXForm );

		Matrix4 color = info.color;
		color.SetCol(2, base);
		//color.Set(3, 2, 0.7f);

		model[HUMAN_MODEL]->SetColorMap( color );
		model[HUMAN_MODEL]->SetBoneFilter( info.filterName, info.filter );
	}

	for (int i = TEMPLE_MODEL; i <= SLEEPTUBE_MODEL; ++i) {
		ProcRenderInfo info;
		// Use to get the base xform, etc.
		AssignProcedural( "team", false, 0, TEAM_HOUSE, false, 0, 0, &info ); 
		model[i]->SetTextureXForm( info.te.uvXForm );

		Matrix4 color = info.color;
		color.SetCol(0, buildBase);
		color.SetCol(1, buildContrast);
		color.SetCol(2, buildGlow);

		model[i]->SetColorMap( color );
		model[i]->SetBoneFilter( info.filterName, info.filter );
	}

	for(int i=RING_0_MODEL; i<=RING_1_MODEL; ++i) {
		ProcRenderInfo info;
		// Use to get the base xform, etc.
		AssignProcedural( "ring", false, i, TEAM_HOUSE, false, 0, 0, &info ); 
		model[i]->SetTextureXForm( info.te.uvXForm );

		Matrix4 color = info.color;
		color.SetCol(0, base);
		color.SetCol(1, contrast);
		model[i]->SetColorMap( color );
		model[i]->SetBoneFilter( info.filterName, info.filter );
	}

	for(int i=PISTOL_MODEL; i<=BEAMGUN_MODEL; ++i) {
		ProcRenderInfo info;
		// Use to get the base xform, etc.
		static const int EFFECT_ARR[4] = { 0, GameItem::EFFECT_FIRE, GameItem::EFFECT_SHOCK, GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK };
		AssignProcedural(GUN_NAME[i - PISTOL_MODEL], false, 0, TEAM_HOUSE, false, EFFECT_ARR[i-PISTOL_MODEL], 0, &info);
		model[i]->SetTextureXForm( info.te.uvXForm );

		Matrix4 color = info.color;
		color.SetCol(0, base);
		color.SetCol(1, contrast);
		model[i]->SetColorMap( color );
		model[i]->SetBoneFilter( info.filterName, info.filter );
	}

	for (int i = 0; i < NUM_PAL_SEL; ++i) {
		CStr<32> str;
		str.Format("(%d,%d)", paletteVec[i].x, paletteVec[i].y);
		paletteLabel[i].SetText(str.safe_str());
	}
}


void ColorTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		Process3DTap( action, view, world, engine );
	}
}


void ColorTestScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	for (int i = 0; i < NUM_PAL_SEL; ++i) {
		if (item == &paletteSelector[i]) {
			float x = 0, y = 0;
			gamui2D.GetRelativeTap(&x, &y);
			paletteVec[i].Set(int(x*float(PAL_COUNT*2)), int(y*float(PAL_COUNT)));
			if (paletteVec[i].x/2 >= paletteVec[i].y) {
				paletteVec[i].x &= (~1);
			}
			DoProcedural();
		}
	}
}


void ColorTestScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void ColorTestScene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void ColorTestScene::HandleHotKey( int mask )
{
	if (mask == GAME_HK_SPACE) {
		cameraHigh = !cameraHigh;
		SetCamera();
	}
	else {
		super::HandleHotKey(mask);
	}
}


void ColorTestScene::Draw3D(U32 deltaTime)
{
	engine->Draw(deltaTime);
}

void ColorTestScene::DrawDebugText()
{
	DrawDebugTextDrawCalls( 0, 16, engine );
}

