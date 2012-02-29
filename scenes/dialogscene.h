#ifndef DIALOGSCENE_INCLUDED
#define DIALOGSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;


class DialogScene : public Scene
{
public:
	DialogScene( LumosGame* game );
	virtual ~DialogScene() {}

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D )
	{
		return RENDER_2D;	
	}
	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	LumosGame* lumosGame;
	gamui::PushButton okay;
};

#endif // DIALOGSCENE_INCLUDED