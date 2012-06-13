#ifndef ANIMATION_SCENE_INCLUDED
#define ANIMATION_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;
class Engine;
class Model;

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
	virtual void Draw3D( U32 deltaTime );

private:
	Engine* engine;
	Model* model;

	gamui::PushButton okay;
};



#endif // ANIMATION_SCENE_INCLUDED