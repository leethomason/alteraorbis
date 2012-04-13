#ifndef NAVTESTSCENE_INCLUDED
#define NAVTESTSCENE_INCLUDED

#include "../xegame/scene.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTestScene : public Scene
{
public:
	NavTestScene( LumosGame* game );
	~NavTestScene();

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D | RENDER_3D;	
	}

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );

private:
	gamui::PushButton okay;
	Engine* engine;
	WorldMap* map;
};


#endif // NAVTESTSCENE_INCLUDED