#ifndef NAVTEST2_SCENE_INCLUDED
#define NAVTEST2_SCENE_INCLUDED

#include "../grinliz/glrandom.h"
#include "../xegame/scene.h"
#include "../xegame/chitbag.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTest2Scene : public Scene, public IChitListener
{
public:
	NavTest2Scene( LumosGame* game );
	~NavTest2Scene();

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D ) { return RENDER_2D | RENDER_3D; }
	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );

	virtual void OnChitMsg( Chit* chit, const char* componentName, int id );

private:
	void LoadMap();

	gamui::PushButton okay;

	Engine* engine;
	WorldMap* map;
	grinliz::Random random;

	ChitBag chitBag;
};



#endif // NAVTEST2_SCENE_INCLUDED
