#ifndef WORLDGEN_SCENE_INCLUDED
#define WORLDGEN_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/texture.h"
#include "../script/worldgen.h"

class LumosGame;
class WorldMap;

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
	WorldGen	worldGen;
	WorldMap*	worldMap;
	U16*		pix16;
	grinliz::CDynArray< WorldFeature > featureArr;

	struct GenState {
		void Clear() { scanline=-1; zone=0; }
		int scanline;	// -1: not started, [0,y-1] scanning, y done scanning
		int zone;		
	};
	GenState genState;
	gamui::PushButton	okay, cancel, tryAgain;
	gamui::Image		worldImage;
	gamui::TextLabel	label;
};

#endif // WORLDGEN_SCENE_INCLUDED