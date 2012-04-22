#ifndef NOISETEST_SCENE_INCLUDED
#define NOISETEST_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;


class NoiseTestScene : public Scene
{
public:
	NoiseTestScene( LumosGame* game );
	virtual ~NoiseTestScene() {}

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D;	
	}
	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	gamui::PushButton okay;
};

#endif // NOISETEST_SCENE_INCLUDED