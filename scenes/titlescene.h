#ifndef TITLESCENE_INCLUDED
#define TITLESCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;


class TitleScene : public Scene
{
public:
	TitleScene( LumosGame* game );
	virtual ~TitleScene() {}

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	LumosGame*		lumosGame;
	gamui::Image	background;

	enum { 
		TEST_DIALOG,
		TEST_RENDER_0,
		TEST_RENDER_1,
		TEST_PARTICLE,
		TEST_NAV,
		TEST_NAV2,
		TEST_NAV_WORLD,
		TEST_NOISE,
		TEST_BATTLE,
		TEST_ANIMATION,
		NUM_TESTS,
		TESTS_PER_ROW = 6
	};
	gamui::PushButton	testScene[NUM_TESTS];
};

#endif // TITLESCENE_INCLUDED