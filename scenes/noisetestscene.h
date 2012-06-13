#ifndef NOISETEST_SCENE_INCLUDED
#define NOISETEST_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glrandom.h"
#include "../engine/texture.h"

class LumosGame;


class NoiseTestScene : public Scene, public ITextureCreator
{
public:
	NoiseTestScene( LumosGame* game );
	virtual ~NoiseTestScene() {}

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

	void CreateTexture( Texture* t );

private:
	gamui::PushButton okay;
	gamui::Image noiseImage;

	enum { SIZE = 128 };
	U16 buffer[SIZE*SIZE];
	grinliz::Random random;
};

#endif // NOISETEST_SCENE_INCLUDED