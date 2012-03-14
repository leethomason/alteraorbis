#ifndef RENDERTESTSCENE_INCLUDED
#define RENDERTESTSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/engine.h"

class LumosGame;


class RenderTestSceneData : public SceneData
{
public:
	RenderTestSceneData( int _id ) : id( _id ) {}
	int id;
};


class RenderTestScene : public Scene
{
public:
	RenderTestScene( LumosGame* game, const RenderTestSceneData* data );
	virtual ~RenderTestScene();

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D | RENDER_3D;	
	}
	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )	{ ProcessTap( action, screen, world ); }
	virtual void ItemTapped( const gamui::UIItem* item );

	virtual void Draw3D();

private:
	enum { NUM_ITEMS = 4,
		   NUM_MODELS = 8 };

	void SetupTest0();
	void SetupTest1();

	int testID;
	LumosGame* lumosGame;
	gamui::PushButton okay;

	Engine* engine;
	Model*  model[NUM_MODELS];
};

#endif // RENDERTESTSCENE_INCLUDED