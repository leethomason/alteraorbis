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

#include <stdio.h>

class Game;
class TiXmlElement;
class Unit;

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

	virtual bool CanSave()										{ return false; }
	virtual void Save( tinyxml2::XMLPrinter* )					{}
	virtual void Load( const tinyxml2::XMLElement* doc )		{}
	virtual void HandleHotKeyMask( int mask )					{}
	virtual void Resize()										{}

	virtual void SceneResult( int sceneID, int result )			{}
	virtual void ChildActivated( int childID, Scene* childScene, SceneData* data )		{}
	Game* GetGame() { return game; }

	// Rendering
	enum {
		RENDER_2D = 0x01,
		RENDER_3D = 0x02
	};
	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D ) = 0;

	// Utility
	void ProcessTap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item )				{}
	virtual gamui::RenderAtom DragStart( const gamui::UIItem* item )	{ gamui::RenderAtom atom; return atom; }	// null atom
	virtual void DragEnd( const gamui::UIItem* item )					{}

	//// ------- public interface  below this line needs eval ---- //
	void RenderGamui2D()	{ gamui2D.Render(); }
	void RenderGamui3D()	{ gamui3D.Render(); }

	// Perspective rendering.
	virtual void DoTick( U32 currentTime, U32 deltaTime )		{}
	// 2D overlay rendering.
	virtual void DrawHUD()										{}

	// Put in debugging output into the 3D stream, or other
	// scene specific 3D elements
	virtual void Draw3D()										{}

protected:

	Game*			game;
	UIRenderer		uiRenderer;
	gamui::Gamui	gamui2D, gamui3D;
};


#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif // UFOATTACK_SCENE_INCLUDED