#ifndef PARTICLESCENE_INCLUDED
#define PARTICLESCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/particle.h"
#include "../grinliz/glcontainer.h"


class LumosGame;
class Engine;
class TestMap;
class Model;


class ParticleScene : public Scene
{
public:
	ParticleScene( LumosGame* game );
	virtual ~ParticleScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 delatTime );

private:
	void Rescan();

	gamui::PushButton okay;
	Engine* engine;
	TestMap* testMap;

	void Clear();
	void Load();

	grinliz::CDynArray< ParticleDef > particleDefArr;
	grinliz::CDynArray< gamui::Button* > buttonArr;
};

#endif // PARTICLESCENE_INCLUDED