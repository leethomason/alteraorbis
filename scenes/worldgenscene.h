#ifndef WORLDGEN_SCENE_INCLUDED
#define WORLDGEN_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/texture.h"

#include "../script/worldgen.h"
#include "../script/rockgen.h"

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
	enum { 
		NZONE = 4,
		NUM_ZONES = NZONE*NZONE
	};

	void BlendLine( int y );

	WorldGen*	worldGen;
	RockGen*	rockGen;
	WorldMap*	worldMap;
	U16*		pix16;

	struct GenState {
		void Clear() { mode=NOT_STARTED; y=0; }
		enum {
			NOT_STARTED,
			WORLDGEN,
			ROCKGEN_START,
			ROCKGEN,
			DONE
		};
		int mode;
		int y;
	};
	GenState genState;
	gamui::PushButton	okay, cancel, tryAgain;
	gamui::Image		worldImage;
	gamui::TextLabel	label;
};

#endif // WORLDGEN_SCENE_INCLUDED