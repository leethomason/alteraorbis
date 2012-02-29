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

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D;	
	}
	virtual void Resize();


private:
	LumosGame* lumosGame;
	gamui::TextLabel label;
	gamui::Image background;
	gamui::PushButton okay, cancel;
};

#endif // TITLESCENE_INCLUDED