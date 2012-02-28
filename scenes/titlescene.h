#ifndef TITLESCENE_INCLUDED
#define TITLESCENE_INCLUDED

#include "../xegame/scene.h"


class TitleScene : public Scene
{
public:
	TitleScene( LumosGame* game ) : Scene( game ) {}
	virtual ~TitleScene() {}

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D;	
	}
};

#endif // TITLESCENE_INCLUDED