#ifndef MAP_SCENE_INCLUDED
#define MAP_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../game/gamelimits.h"
#include "../game/lumosmath.h"
#include "../widget/mapgridwidget.h"

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
		player(pc),
		view(false)
	{
		destSector.Zero();
		for (int i = 0; i < MAX_SQUADS; ++i) squadDest[i].Zero();
	}

	LumosChitBag*	lumosChitBag;
	WorldMap*		worldMap;
	Chit*			player;

	grinliz::Vector2I	squadDest[MAX_SQUADS];

	// Read/Write	
	grinliz::Vector2I	destSector;	// fast travel. request going in, destination coming out.
	bool				view;		// if true, request is to view, not grid travel.
};


class MapScene : public Scene
{
public:
	MapScene( LumosGame* game, MapSceneData* data );
	~MapScene();

	virtual void Resize();
	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		return ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void HandleHotKey( int value );

private:
	grinliz::Rectangle2I MapBounds2();
	grinliz::Rectangle2F GridBounds2(int x, int y, bool gutter);
	grinliz::Vector2F ToUI(int select, const grinliz::Vector2F& pos, const grinliz::Rectangle2I& bounds, bool* inBounds);
	void DrawMap();

	enum {	MAP2_RAD	= 2,
			MAP2_SIZE	= MAP2_RAD*2+1,
			MAP2_SIZE2	= MAP2_SIZE*MAP2_SIZE,
			MAX_COL		= 3,
			MAX_FACE	= MAP2_SIZE2 * MAX_COL * 2
	};

	LumosGame*			lumosGame;
	LumosChitBag*		lumosChitBag;
	WorldMap*			worldMap;
	Chit*				player;
	MapSceneData*		data;

	gamui::PushButton	okay;
	gamui::PushButton	gridTravel, viewButton;
	gamui::Image		mapImage;
	gamui::Image		mapImage2;

	gamui::Image		playerMark[2];
	gamui::Image		homeMark[2];
	gamui::Image		travelMark;
	gamui::Image		squadMark[2][MAX_SQUADS];
	gamui::Image		selectionMark;
	gamui::Image		diplomacy[NUM_SECTORS*NUM_SECTORS];
	gamui::Canvas		webCanvas;
	gamui::Image		unitMarker[MAX_CITIZENS];

	MapGridWidget		gridWidget[MAP2_SIZE2];
};

#endif // MAP_SCENE_INCLUDED
