#ifndef FORGE_SCENE_INCLUDE
#define FORGE_SCENE_INCLUDE

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class GameItem;
class LumosGame;


class ForgeSceneData : public SceneData
{
public:
	ForgeSceneData() : SceneData(), item(0) {}
	GameItem* item;		// item that was forged.
};


class ForgeScene : public Scene
{
public:
	ForgeScene( LumosGame* game, ForgeSceneData* data );
	~ForgeScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

	virtual void Draw3D( U32 delatTime );

private:
	LumosGame*			lumosGame;

	gamui::PushButton	okay;

	enum {
		RING,
		GUN,
		SHIELD,
		NUM_ITEM_TYPES
	};

	enum {
		PISTOL,
		BLASTER,
		PULSE,
		BEAMGUN,
		NUM_GUN_TYPES
	};

	gamui::ToggleButton	itemType[NUM_ITEM_TYPES];
	gamui::ToggleButton gunType[NUM_GUN_TYPES];
};


#endif // FORGE_SCENE_INCLUDE
