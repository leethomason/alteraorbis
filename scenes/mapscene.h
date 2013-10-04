#ifndef MAP_SCENE_INCLUDED
#define MAP_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;
class LumosChitBag;
class WorldMap;
class Chit;


class MapSceneData : public SceneData
{
public:
	MapSceneData( LumosChitBag* lcb, WorldMap* wm, Chit* pc ) : 
		SceneData(),
		lumosChitBag(lcb),
		worldMap(wm),
		player(pc) {}

	LumosChitBag*	lumosChitBag;
	WorldMap*		worldMap;
	Chit*			player;
};

class MapScene : public Scene
{
public:
	MapScene( LumosGame* game, MapSceneData* data );
	~MapScene();

	virtual void Resize();
	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	LumosGame*		lumosGame;
	LumosChitBag*	lumosChitBag;
	WorldMap*		worldMap;
	Chit*			player;

	grinliz::Rectangle2F map2Bounds;

	gamui::PushButton	okay;
	gamui::Image		mapImage;
	gamui::Image		map2Image;

	gamui::Image		playerMark;
};

#endif // MAP_SCENE_INCLUDED
