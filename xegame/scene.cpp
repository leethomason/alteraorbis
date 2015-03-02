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

#include "game.h"
#include "scene.h"
#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"			// used for 3D dragging and debugging. Scene should *not* have an engine requirement.
#include "../engine/text.h"
#include "../game/layout.h"			// used for the text height
#include "../engine/particle.h"
#include "../audio/xenoaudio.h"
#include "../xegame/istringconst.h"
#include "../game/lumosgame.h"

using namespace grinliz;
using namespace gamui;


Scene::Scene( Game* _game )
	: game( _game ),
	  uiRenderer( GPUDevice::HUD )
{

	gamui2D.Init(&uiRenderer);
	FontSingleton* bridge = FontSingleton::Instance();

	// Don't set text height on scene constructor. Do in Resize()
	//int heightInPixels = (int)gamui2D.TransformVirtualToPhysical(float(LAYOUT_TEXT_HEIGHT));
	//bridge->SetPhysicalPixel(heightInPixels);
	gamui2D.SetText(bridge->TextAtom(false), bridge->TextAtom(true), FontSingleton::Instance());

	const Screenport& screenport = _game->GetScreenport();
	ResizeGamui(screenport.PhysicalWidth(), screenport.PhysicalHeight());
	
	RenderAtom nullAtom;
	dragImage.Init( &gamui2D, nullAtom, true );
	threeDTapDown = false;
}


bool Scene::ProcessTap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	grinliz::Vector2F window;
	game->GetScreenport().ViewToWindow( view, &window );
	bool tapCaptured = (gamui2D.TapCaptured() != 0);

	// Callbacks:
	//		ItemTapped
	//		DragStart
	//		DragEnd

	const UIItem* uiItem = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( window.x, window.y );
		dragStarted = false;
		return gamui2D.TapCaptured() != 0;
	}
	else if ( action == GAME_TAP_MOVE ) {
		if ( !dragStarted ) {
			if ( gamui2D.TapCaptured() ) {
				dragStarted = true;
				RenderAtom atom = DragStart( gamui2D.TapCaptured() );
				dragImage.SetAtom( atom );
			}
		}
		dragImage.SetCenterPos(gamui2D.TransformPhysicalToVirtual(window.x), gamui2D.TransformPhysicalToVirtual(window.y));
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		dragImage.SetAtom( RenderAtom() );
		dragStarted = false;
		return tapCaptured;		// whether it was captured.
	}
	else if ( action == GAME_TAP_UP ) {
		const UIItem* dragStart = gamui2D.TapCaptured();
		uiItem = gamui2D.TapUp( window.x, window.y );
		
		if ( dragStarted ) {
			dragStarted = false;
			const UIItem* start = 0;
			const UIItem* end   = 0;
			gamui2D.GetDragPair( &start, &end ); 
			this->DragEnd( start, end );
			dragImage.SetAtom( RenderAtom() );
		}
	}

	if ( uiItem ) {
		XenoAudio::Instance()->Play(ISC::buttonWAV, 0);
		ItemTapped(uiItem);
	}
	return tapCaptured;
}


ModelVoxel Scene::ModelAtMouse( const grinliz::Vector2F& view, 		
								Engine* engine,
								HitTestMethod method,
								int required, int exclude, 
								const Model * const * ignore, 
								Vector3F* planeIntersection ) const
{ 
	Matrix4 mvpi;
	Ray ray;
	Vector3F at = { 0,0,0 };
	game->GetScreenport().ViewProjectionInverse3D( &mvpi );
	engine->RayFromViewToYPlane( view, mvpi, &ray, &at );
	ModelVoxel mv = engine->IntersectModelVoxel( ray.origin, ray.direction, 10000.0f, 
												method, required, exclude, ignore );
	if ( planeIntersection ) {
		*planeIntersection = at;
	}
	return mv;
}


grinliz::Color4F Scene::ClearColor()
{
	const Game::Palette* pal = Game::GetMainPalette();
	Color4F c = pal->Get4F(0, 4);
	c.x *= 0.25f;
	c.y *= 0.25f;
	c.z *= 0.25f;
	return c;
}


bool Scene::Process3DTap(int action, const grinliz::Vector2F& view, const grinliz::Ray& world, Engine* engine)
{
	Ray ray;
	bool result = false;
		
	switch( action )
	{
	case GAME_PAN_START:
		{
			game->GetScreenport().ViewProjectionInverse3D( &dragData3D.mvpi );
			engine->RayFromViewToYPlane( view, dragData3D.mvpi, &ray, &dragData3D.start3D );
			dragData3D.startCameraWC = engine->camera.PosWC();
			dragData3D.end3D = dragData3D.start3D;
			dragData3D.start2D = dragData3D.end2D = view;
			threeDTapDown = true;
			break;
		}

	case GAME_PAN_MOVE:
	case GAME_PAN_END:
		{
			// Not sure how this happens. Why is the check for down needed??
			if( threeDTapDown ) {
				Vector3F drag;
				engine->RayFromViewToYPlane( view, dragData3D.mvpi, &ray, &drag );
				dragData3D.end2D = view;

				Vector3F delta = drag - dragData3D.start3D;
				GLASSERT( fabsf( delta.x ) < 1000.f && fabsf( delta.y ) < 1000.0f );
				delta.y = 0;
				drag.y = 0;
				dragData3D.end3D = drag;

				if ( action == GAME_TAP_UP ) {
					threeDTapDown = false;
					Vector2F vDelta = dragData3D.start2D - dragData3D.end2D;
					if ( vDelta.Length() < 10.f )
						result = true;
				}
				engine->camera.SetPosWC(dragData3D.startCameraWC - delta);
			}
			break;
		}
	}
	return result;
}


void Scene::DrawDebugTextDrawCalls( int x, int y, Engine* engine )
{
	UFOText* ufoText = UFOText::Instance();
	ufoText->Draw( x, y, "Model Draw Calls glow-em=%d shadow=%d model=%d nPart=%d",
		engine->modelDrawCalls[Engine::GLOW_EMISSIVE],
		engine->modelDrawCalls[Engine::SHADOW],
		engine->modelDrawCalls[Engine::MODELS],
		engine->particleSystem->NumParticles() );
}


void Scene::MoveImpl(float dx, float dy, Engine* engine)
{
	Camera& camera = engine->camera;
	const Vector3F* dir = camera.EyeDir3();

	Vector3F right = dir[2];
	Vector3F forward = dir[0];
	right.y = 0;
	forward.y = 0;
	right.Normalize();
	forward.Normalize();

	Vector3F pos = camera.PosWC();
	pos = pos + dx*right + dy*forward;

	camera.SetPosWC(pos);
}


void Scene::HandleHotKey( int value )
{
	Engine* engine = GetEngine();
	int stage = 0;

	if ( value == GAME_HK_TOGGLE_GLOW ) {
		stage = Engine::STAGE_GLOW; 
	}
	else if ( value == GAME_HK_TOGGLE_PARTICLE ) {
		stage = Engine::STAGE_PARTICLE;
	}
	else if ( value == GAME_HK_TOGGLE_VOXEL ) {
		stage = Engine::STAGE_VOXEL;
	}
	else if ( value == GAME_HK_TOGGLE_SHADOW ) {
		stage = Engine::STAGE_SHADOW;
	}
	else if ( value == GAME_HK_TOGGLE_BOLT ) {
		stage = Engine::STAGE_BOLT;
	}

	if ( engine && stage ) {
		int s = engine->Stages();
		if ( s & stage ) {
			s = s & (~stage);
		}
		else {
			s |= stage;
		}
		engine->SetStages( s );
	}
}


/*
640x480 mininum screen.
6 buttons
80 pixels / per
*/
gamui::LayoutCalculator Scene::DefaultLayout()
{
	LayoutCalculator layout(gamui2D.Width(), gamui2D.Height());
	layout.SetGutter(LAYOUT_GUTTER, LAYOUT_GUTTER);
	layout.SetSize(LAYOUT_SIZE_X, LAYOUT_SIZE_Y);
	layout.SetSpacing(LAYOUT_SPACING);
	return layout;
}



void Scene::InitStd(gamui::Gamui* g, gamui::PushButton* okay, gamui::PushButton* cancel)
{
	const ButtonLook& stdBL = static_cast<LumosGame*>(game)->GetButtonLook(LumosGame::BUTTON_LOOK_STD);
	gamui::LayoutCalculator layout = DefaultLayout();

	if (okay) {
		okay->Init(g, stdBL);
		okay->SetSize(LAYOUT_SIZE_X, LAYOUT_SIZE_Y);
		okay->SetDeco(LumosGame::CalcUIIconAtom("okay", true), LumosGame::CalcUIIconAtom("okay", false));
	}
	if (cancel) {
		cancel->Init(g, stdBL);
		cancel->SetSize(LAYOUT_SIZE_X, LAYOUT_SIZE_Y);
		cancel->SetDeco(LumosGame::CalcUIIconAtom("cancel", true), LumosGame::CalcUIIconAtom("cancel", false));
	}
}


void Scene::PositionStd(gamui::PushButton* okay, gamui::PushButton* cancel)
{
	gamui::LayoutCalculator layout = DefaultLayout();

	if (okay)
		layout.PosAbs(okay, OKAY_X, -1);

	if (cancel)
		layout.PosAbs(cancel, CANCEL_X, -1);
}


void Scene::ResizeGamui(int w, int h)
{
	gamui2D.SetScale(w, h, LAYOUT_VIRTUAL_HEIGHT);

	FontSingleton* bridge = FontSingleton::Instance();

	// Generate / reset the text.
	int heightInPixels = (int)gamui2D.TransformVirtualToPhysical(float(LAYOUT_TEXT_HEIGHT));
	bridge->SetPhysicalPixel(heightInPixels);

	// Set the atoms to the gamui system:
	gamui2D.SetText(bridge->TextAtom(false), bridge->TextAtom(true), FontSingleton::Instance());
}
