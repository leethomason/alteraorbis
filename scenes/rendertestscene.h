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

#ifndef RENDERTESTSCENE_INCLUDED
#define RENDERTESTSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/engine.h"

class LumosGame;
class Engine;
class TestMap;
class Model;

class RenderTestSceneData : public SceneData
{
public:
	RenderTestSceneData( int _id ) : id( _id ) {}
	int id;
};


class RenderTestScene : public Scene, public IUITracker
{
	typedef Scene super;
public:
	RenderTestScene( LumosGame* game, const RenderTestSceneData* data );
	virtual ~RenderTestScene();

	virtual void Resize();

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Zoom( int style, float normal );
	virtual void Rotate( float degrees );
	virtual void Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world);
	virtual void MoveCamera(float dx, float dy);

	virtual void HandleHotKey( int mask );

	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();

	void UpdateUIElements(const Model* models[], int n);

private:
	enum { NUM_ITEMS = 4,
		   NUM_MODELS = 16,
		   NUM_CONTROL = 6 };

	void SetupTest();
	void LoadLighting();

	int glowLayer;
	LumosGame* lumosGame;
	gamui::PushButton okay;
	gamui::PushButton refreshButton;
	gamui::ToggleButton control[ NUM_CONTROL ];
	gamui::Image rtImage;
	gamui::Image mapImage, headImage;

	Engine* engine;
	Model*  model[NUM_MODELS];
	TestMap* testMap;
};

#endif // RENDERTESTSCENE_INCLUDED