#ifndef BATTLE_SCENE_INCLUDED
#define BATTLE_SCENE_INCLUDED

#include "../grinliz/glrandom.h"
#include "../xegame/scene.h"
#include "../xegame/chitbag.h"

class LumosGame;
class Engine;
class WorldMap;


class BattleTestScene : public Scene, public IChitListener
{
public:
	BattleTestScene( LumosGame* game );
	~BattleTestScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* );
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world ) { debugRay = world; }

private:
	enum {
		HUMAN=1,
		HORNET,
		DUMMY
	};

	void LoadMap();
	void CreateChit( const grinliz::Vector2I& p );

	gamui::PushButton okay, goButton;

	Engine* engine;
	WorldMap* map;

	grinliz::Random random;
	grinliz::CDynArray<grinliz::Vector2I> waypoints;
	grinliz::Ray debugRay;
	
	ChitBag chitBag;
};


#endif // BATTLE_SCENE_INCLUDED
