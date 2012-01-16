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
		SCENE_GAME,
	};

	virtual int LoadSceneID()								{ return SCENE_GAME; }
	virtual Scene* CreateScene( int id, SceneData* data );
	virtual void CreateTexture( Texture* t )				{ GLASSERT( 0 ); }


	static gamui::RenderAtom CalcDecoAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcParticleAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcIconAtom( int id, bool enabled=true );
	static gamui::RenderAtom CalcIcon2Atom( int id, bool enabled=true );
	static gamui::RenderAtom CalcPaletteAtom( int c0, int c1, int blend, bool enable=true );
};


#endif //  LUMOS_LUMOSGAME_INCLUDED