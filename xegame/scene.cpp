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
#include "../engine/engine.h"			// used for 3D dragging; Scene should *not* have an engine.

using namespace grinliz;
using namespace gamui;

Scene::Scene( Game* _game )
	: game( _game )
{
	gamui2D.Init( &uiRenderer, game->GetRenderAtom( Game::ATOM_TEXT ), game->GetRenderAtom( Game::ATOM_TEXT_D ), &uiRenderer );
	gamui3D.Init( &uiRenderer, game->GetRenderAtom( Game::ATOM_TEXT ), game->GetRenderAtom( Game::ATOM_TEXT_D ), &uiRenderer );
	gamui2D.SetTextHeight( 24 );
	gamui3D.SetTextHeight( 24 );
	
	RenderAtom nullAtom;
	dragImage.Init( &gamui2D, nullAtom, true );
}


bool Scene::ProcessTap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )
{
	grinliz::Vector2F ui;
	game->GetScreenport().ViewToUI( screen, &ui );
	bool tapCaptured = (gamui2D.TapCaptured() != 0);

	// Callbacks:
	//		ItemTapped
	//		DragStart
	//		DragEnd

	const UIItem* uiItem = 0;
	if ( action == GAME_TAP_DOWN ) {
		gamui2D.TapDown( ui.x, ui.y );
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
		dragImage.SetCenterPos( ui.x, ui.y );
	}
	else if ( action == GAME_TAP_CANCEL ) {
		gamui2D.TapCancel();
		dragImage.SetAtom( RenderAtom() );
		dragStarted = false;
		return tapCaptured;		// whether it was captured.
	}
	else if ( action == GAME_TAP_UP ) {
		const UIItem* dragStart = gamui2D.TapCaptured();
		uiItem = gamui2D.TapUp( ui.x, ui.y );
		
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
		ItemTapped( uiItem );
	}
	return tapCaptured;
}


void Scene::Process3DTap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world, Engine* engine )
{
	Ray ray;
		
	switch( action )
	{
		case GAME_TAP_DOWN:
		{
			game->GetScreenport().ViewProjectionInverse3D( &dragData3D.mvpi );
			engine->RayFromViewToYPlane( view, dragData3D.mvpi, &ray, &dragData3D.start3D );
			dragData3D.startCameraWC = engine->camera.PosWC();
			dragData3D.end3D = dragData3D.start3D;
			break;
		}

		case GAME_TAP_MOVE:
		case GAME_TAP_UP:
		{
			Vector3F drag;
			engine->RayFromViewToYPlane( view, dragData3D.mvpi, &ray, &drag );

			Vector3F delta = drag - dragData3D.start3D;
			delta.y = 0;
			drag.y = 0;
			dragData3D.end3D = drag;

			engine->camera.SetPosWC( dragData3D.startCameraWC - delta );
			break;
		}
	}
}

