#ifndef LUMOS_GAME_SCENE_INCLUDED
#define LUMOS_GAME_SCENE_INCLUDED

#include "../xegame/scene.h"

class LumosGame;
class Sim;

class GameScene : public Scene
{
	typedef Scene super;
public:
	GameScene( LumosGame* game );
	~GameScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void HandleHotKey( int mask );

	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();

private:
	LumosGame*	lumosGame;
	Sim*		sim;

	gamui::PushButton	okay;
	gamui::Image		minimap;
	gamui::Image		playerMark;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
