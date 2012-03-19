#ifndef PARTICLESCENE_INCLUDED
#define PARTICLESCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/particle.h"


class LumosGame;
class Engine;
class TestMap;
class Model;


class ParticleScene : public Scene
{
public:
	ParticleScene( LumosGame* game );
	virtual ~ParticleScene();

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

	virtual void Draw3D( U32 delatTime );

private:
	gamui::PushButton okay;
	Engine* engine;
	TestMap* testMap;

	void Clear();
	void Load();

	CDynArray< ParticleDef > particleDefArr;
	CDynArray< gamui::Button* > buttonArr;
};

#endif // PARTICLESCENE_INCLUDED