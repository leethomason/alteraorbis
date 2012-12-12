#ifndef WORLDGEN_SCENE_INCLUDED
#define WORLDGEN_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/texture.h"
#include "../script/worldgen.h"

class LumosGame;

class WorldGenScene : public Scene, public ITextureCreator
{
public:
	WorldGenScene( LumosGame* game );
	~WorldGenScene();

	virtual void Resize();
	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void DoTick( U32 step );

	void CreateTexture( Texture* t );

private:
	WorldGen worldGen;
	int scanline;	// -1: not started
	U16*				pixels;
	gamui::PushButton	okay, cancel, tryAgain;
	gamui::Image		worldImage;
	gamui::TextLabel	label;
};

#endif // WORLDGEN_SCENE_INCLUDED