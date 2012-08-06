#ifndef NAVTEST2_SCENE_INCLUDED
#define NAVTEST2_SCENE_INCLUDED

#include "../grinliz/glrandom.h"
#include "../xegame/scene.h"
#include "../xegame/chitbag.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTest2SceneData : public SceneData
{
public:
	NavTest2SceneData( const char* _worldFilename, int _nChits, int _nPerCreation=1 ) : worldFilename( _worldFilename ), nChits( _nChits ), nPerCreation( _nPerCreation ) {}

	const char* worldFilename;
	int nChits;
	int nPerCreation;
};

class NavTest2Scene : public Scene, public IChitListener
{
public:
	NavTest2Scene( LumosGame* game, const NavTest2SceneData* );
	~NavTest2Scene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world ) { debugRay = world; }

private:
	void LoadMap();
	void CreateChit( const grinliz::Vector2I& p );

	gamui::PushButton okay;
	gamui::ToggleButton regionButton;
	gamui::Image minimap;

	Engine* engine;
	WorldMap* map;
	const NavTest2SceneData* data;

	grinliz::Random random;
	grinliz::CDynArray<grinliz::Vector2I> waypoints;
	grinliz::Ray debugRay;
	
	int nChits;
	int creationTick;

	ChitBag chitBag;
};



#endif // NAVTEST2_SCENE_INCLUDED
