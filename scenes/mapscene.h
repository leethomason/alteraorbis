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
		player(pc) 
	{
		destSector.Zero();
	}

	LumosChitBag*	lumosChitBag;
	WorldMap*		worldMap;
	Chit*			player;

	// Read/Write	
	grinliz::Vector2I	destSector;	// fast travel. request going in, destination coming out.
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
	virtual void HandleHotKey( int value );

private:

	void SetText();

	enum {	MAP2_RAD	= 2,
			MAP2_SIZE	= 5,
			MAP2_SIZE2	= MAP2_SIZE*MAP2_SIZE
	};

	LumosGame*		lumosGame;
	LumosChitBag*	lumosChitBag;
	WorldMap*		worldMap;
	Chit*			player;
	MapSceneData*	data;

	grinliz::Rectangle2I sectorBounds;
	grinliz::Rectangle2F map2Bounds;

	gamui::PushButton	okay;
	gamui::PushButton	gridTravel;
	gamui::Image		mapImage;
	gamui::Image		map2Image;

	gamui::Image		playerMark, playerMark2;
	gamui::TextBox		map2Text[MAP2_SIZE2];
};

#endif // MAP_SCENE_INCLUDED
