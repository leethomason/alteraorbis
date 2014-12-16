#ifndef MAP_SCENE_INCLUDED
#define MAP_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../game/gamelimits.h"
#include "../game/lumosmath.h"

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
	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void HandleHotKey( int value );

private:
	grinliz::Rectangle2I MapBounds2();
	grinliz::Rectangle2F GridBounds2(int x, int y, bool gutter);
	grinliz::Vector2F ToUI(int select, const grinliz::Vector2F& pos, const grinliz::Rectangle2I& bounds, bool* inBounds);
	void DrawMap();

	struct MCount {
		MCount() : count(0) {}
		MCount(const grinliz::IString& istr) : name(istr), count(0) {}

		bool operator==(const MCount& rhs) const { return name == rhs.name; }

		grinliz::IString name;
		int				 count;
	};

	struct MCountSorter {
		static bool Less( const MCount& v0, const MCount& v1 )	{ return v0.count > v1.count; }
	};

	enum {	MAP2_RAD	= 2,
			MAP2_SIZE	= 5,
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
	gamui::Image		face[MAX_FACE];

	gamui::Image		playerMark[2];
	gamui::Image		homeMark[2];
	gamui::Image		travelMark[2];
	gamui::Image		squadMark[2][MAX_SQUADS];
	gamui::Image		selectionMark;
	gamui::TextLabel	map2Text[MAP2_SIZE2];
	enum {WHITE_CANVAS, RED_CANVAS, YELLOW_CANVAS, GREEN_CANVAS, NUM_CANVAS};
	gamui::Canvas		webCanvas[NUM_CANVAS];
};

#endif // MAP_SCENE_INCLUDED
