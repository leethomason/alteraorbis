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

#ifndef UFOATTACK_SCENE_INCLUDED
#define UFOATTACK_SCENE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glrectangle.h"

#include "../tinyxml2/tinyxml2.h"

#include "../gamui/gamui.h"

#include "../engine/uirendering.h"
#include "../engine/modelvoxel.h"

#include <stdio.h>

class Game;
class TiXmlElement;
class Unit;
class Engine;
class Model;

#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable : 4100 )	// un-referenced formal parameter
#endif

class SceneData
{
public:
	SceneData()				{}
	virtual ~SceneData()	{}
};


struct DragData {
	grinliz::Matrix4  mvpi;
	grinliz::Vector3F start3D, end3D;
	grinliz::Vector2F start2D, end2D;
	grinliz::Vector3F startCameraWC;
};


/**
*/
class Scene
{
public:
	Scene( Game* _game );
	virtual ~Scene()											{}

	virtual void Activate()										{}
	virtual void DeActivate()									{}

	// UI
	virtual void Tap(	int action, 
						const grinliz::Vector2F& screen,
						const grinliz::Ray& world )				{}
	virtual void Zoom( int style, float normal )				{}
	virtual void Rotate( float degrees )						{}
	virtual void CancelInput()									{}
	// Only with mouse/kbd input.
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world )	{}

	virtual void HandleHotKey( int mask )						{}
	virtual void Resize()										{}

	virtual void SceneResult( int sceneID, int result )			{}
	virtual void ChildActivated( int childID, Scene* childScene, SceneData* data )		{}
	Game* GetGame() { return game; }

	// Rendering
	virtual void DoTick( U32 deltaTime )						{}
	virtual void Draw3D( U32 deltaTime )						{}
	virtual grinliz::Color4F ClearColor()						{ grinliz::Color4F c = { 0,0,0,0 }; return c; }

	// Utility
	
	// Call to send mouse events to gamui. Returns true if the UI (gamui) is handling the event.
	bool ProcessTap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world );
	// Call to handle 3D events. Returns true if an actual tap (on the y=0 plane) occurs.
	bool Process3DTap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world, Engine* engine );
	ModelVoxel ModelAtMouse( const grinliz::Vector2F& view, 
							 Engine* engine,
							 HitTestMethod method = TEST_TRI,
							 int required=0, int exclude=0, 
							 const Model * const * ignore=0, 
							 grinliz::Vector3F* planeIntersection = 0 ) const;

	virtual void ItemTapped( const gamui::UIItem* item )							{}
	virtual gamui::RenderAtom DragStart( const gamui::UIItem* item )				{ gamui::RenderAtom atom; return atom; }	// null atom
	virtual void DragEnd( const gamui::UIItem* start, const gamui::UIItem* end )	{}

	//// ------- public interface  below this line needs eval ---- //
	void RenderGamui2D()	{ gamui2D.Render(); }

	// 2D overlay rendering.
	virtual void DrawDebugText()								{}

protected:

	// Utility function for standard output:
	void DrawDebugTextDrawCalls( int x, int y, Engine* engine );

	Game*			game;
	UIRenderer		uiRenderer;
	gamui::Gamui	gamui2D;
	gamui::Image	dragImage;
	bool			dragStarted;
	bool			threeDTapDown;
	bool			enable3DDragging;

	DragData		dragData3D;
};




#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif // UFOATTACK_SCENE_INCLUDED