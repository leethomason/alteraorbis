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

#ifndef LIVE_PREVIEW_SCENE_INCLUDED
#define LIVE_PREVIEW_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../engine/texture.h"

class LumosGame;
class Engine;
class Model;


class LivePreviewSceneData : public SceneData
{
public:
	LivePreviewSceneData( bool isLive ) : live( isLive ) {}
	bool live;
};


class LivePreviewScene : public Scene
{
public:
	LivePreviewScene( LumosGame* game, const LivePreviewSceneData* data );
	virtual ~LivePreviewScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );
	virtual void DoTick( U32 deltaTime );
	virtual grinliz::Color4F ClearColor();

private:
	void CreateTexture( int type );
	void GenerateFaces( int mainRow );
	void GenerateRing( int mainRow );

	enum { 
		ROWS = 4, 
		COLS = 5, 
		NUM_MODEL = ROWS*COLS,

		HUMAN_MALE_FACE = 0,
		HUMAN_FEMALE_FACE,
		RING,
		NUM_TYPES
	};

	gamui::PushButton okay;
	gamui::ToggleButton rowButton[ROWS];
	gamui::ToggleButton typeButton[NUM_TYPES];

	U32 fileTimer;
	U32 timer;
	time_t fileTime;

	bool		live;
	int			currentType;
	Engine*		engine;
	Model*		model[NUM_MODEL];
};

#endif // LIVE_PREVIEW_SCENE_INCLUDED