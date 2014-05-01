#ifndef Credits_SCENE_INCLUDED
#define Credits_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../script/itemscript.h"

class LumosGame;

class CreditsScene : public Scene
{
public:
	CreditsScene(LumosGame* game);
	virtual ~CreditsScene();

	virtual void Resize();

	virtual void Tap(int action, const grinliz::Vector2F& screen, const grinliz::Ray& world)
	{
		ProcessTap(action, screen, world);
	}
	virtual void ItemTapped(const gamui::UIItem* item);
	virtual grinliz::Color4F ClearColor();

private:
	gamui::PushButton okay;
	gamui::TextLabel text;
};

#endif // Credits_SCENE_INCLUDED

