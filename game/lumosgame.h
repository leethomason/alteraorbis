#ifndef LUMOS_LUMOSGAME_INCLUDED
#define LUMOS_LUMOSGAME_INCLUDED

#include "../xegame/game.h"

class LumosGame : public Game
{
public:
	enum {
		DECO_NONE = 32
	};

	enum {
		PALETTE_NORM, PALETTE_BRIGHT, PALETTE_DARK
	};

	LumosGame(  int width, int height, int rotation, const char* savepath );
	virtual ~LumosGame();

	enum {
		SCENE_TITLE,
		SCENE_DIALOG,
		SCENE_RENDERTEST,
		SCENE_GAME
	};

	virtual int LoadSceneID()								{ return SCENE_GAME; }
	virtual Scene* CreateScene( int id, SceneData* data );
	virtual void CreateTexture( Texture* t )				{ GLASSERT( 0 ); }

	enum {
		DECO_OKAY = 15,
		DECO_CANCEL = 22
	};
	static gamui::RenderAtom CalcDecoAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcParticleAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcIconAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcIcon2Atom( int id, bool enabled=true );
	static gamui::RenderAtom CalcPaletteAtom( int c0, int c1, int blend, bool enable=true );

	enum {
		BUTTON_LOOK_STD,
		BUTTON_LOOK_COUNT
	};
	const gamui::ButtonLook& GetButtonLook( int i ) { GLASSERT( i>=0 && i<BUTTON_LOOK_COUNT ); return buttonLookArr[i]; }
	
	enum {
		CANCEL_X = -1,
		OKAY_X = 0
	};
	gamui::LayoutCalculator DefaultLayout();
	void InitStd( gamui::Gamui* g, gamui::PushButton* okay, gamui::PushButton* cancel );
	void PositionStd( gamui::PushButton* okay, gamui::PushButton* cancel );

private:
	void InitButtonLooks();

	gamui::ButtonLook buttonLookArr[BUTTON_LOOK_COUNT];
};


#endif //  LUMOS_LUMOSGAME_INCLUDED