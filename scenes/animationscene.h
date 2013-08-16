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

#ifndef ANIMATION_SCENE_INCLUDED
#define ANIMATION_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glcontainer.h"
#include "../engine/animation.h"

class LumosGame;
class Engine;
class Model;
class ModelResource;

class AnimationScene : public Scene
{
public:
	AnimationScene( LumosGame* game );
	virtual ~AnimationScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	void Zoom( int style, float delta );
	void Rotate( float degrees );
	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 deltaTime );

private:
	void LoadModel();
	void SetModelVis( bool onlyShowOne );
	void UpdateBoneInfo();
	void UpdateAnimationInfo();
	void UpdateModelInfo();

	Engine* engine;
	enum { NUM_MODELS = 3 };

	grinliz::CDynArray< const ModelResource* > resourceArr;
	Model* model[ NUM_MODELS ];
	Model* triggerModel;

	int currentModel;
	int  currentBone;
	int  currentAnim;

	gamui::PushButton okay;
	gamui::PushButton boneLeft, boneRight;
	gamui::PushButton modelLeft, modelRight;
	gamui::ToggleButton animSelect[ANIM_COUNT];

	gamui::ToggleButton ortho;
	gamui::ToggleButton zeroFrame;
	gamui::ToggleButton instance;
	enum { PARTICLE, GUN, RING, LARGE_RING, NUM_TRIGGERS };
	gamui::ToggleButton triggerToggle[NUM_TRIGGERS];

	gamui::TextLabel boneName, modelName;

	grinliz::Vector2I origin;
	grinliz::Rectangle2I size;
	grinliz::Rectangle2I partSize[EL_MAX_BONES];
};



#endif // ANIMATION_SCENE_INCLUDED