#include "lumosgame.h"
#include "gamelimits.h"

#include "../scenes/titlescene.h"
#include "../scenes/dialogscene.h"
#include "../scenes/rendertestscene.h"
#include "../scenes/particlescene.h"
#include "../scenes/navtestscene.h"
#include "../scenes/navtest2scene.h"
#include "../scenes/noisetestscene.h"

using namespace grinliz;
using namespace gamui;


LumosGame::LumosGame(  int width, int height, int rotation, const char* savepath ) 
	: Game( width, height, rotation, 480, savepath )
{
	InitButtonLooks();

	PushScene( SCENE_TITLE, 0 );
	PushPopScene();
}


LumosGame::~LumosGame()
{}


void LumosGame::InitButtonLooks()
{
	TextureManager* tm = TextureManager::Instance();

	RenderAtom blueUp(	(const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
						(const void*)tm->GetTexture( "icons" ), 
						0.125f, 0, 0.250f, 0.250f );
	RenderAtom blueUpD = blueUp;
	blueUpD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	RenderAtom blueDown((const void*)UIRenderer::RENDERSTATE_UI_NORMAL, 
						(const void*)tm->GetTexture( "icons" ), 
						0.125f, 0.250f, 0.250f, 0.500f );
	RenderAtom blueDownD = blueDown;
	blueDownD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

	buttonLookArr[ BUTTON_LOOK_STD ].Init( blueUp, blueUpD, blueDown, blueDownD );
}


Scene* LumosGame::CreateScene( int id, SceneData* data )
{
	Scene* scene = 0;

	switch ( id ) {
	case SCENE_TITLE:		scene = new TitleScene( this );				break;
	case SCENE_DIALOG:		scene = new DialogScene( this );			break;
	case SCENE_RENDERTEST:	scene = new RenderTestScene( this, (const RenderTestSceneData*)data );	break;
	case SCENE_PARTICLE:	scene = new ParticleScene( this );			break;
	case SCENE_NAVTEST:		scene = new NavTestScene( this );			break;
	case SCENE_NAVTEST2:	scene = new NavTest2Scene( this, (const NavTest2SceneData*)data );			break;
	case SCENE_NOISETEST:	scene = new NoiseTestScene( this );			break;

	default:
		GLASSERT( 0 );
		break;
	}
	return scene;
}


void LumosGame::CreateTexture( Texture* t )
{
	Game::CreateTexture( t );
}


RenderAtom LumosGame::CalcParticleAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 16 );
	Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
	int y = id / 4;
	int x = id - y*4;
	float tx0 = (float)x / 4.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/4.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIconAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons" );
	int y = id / 8;
	int x = id - y*8;
	float tx0 = (float)x / 8.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/8.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIcon2Atom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons2" );

	static const int CX = 4;
	static const int CY = 4;

	int y = id / CX;
	int x = id - y*CX;
	float tx0 = (float)x / (float)CX;
	float ty0 = (float)y / (float)CY;;
	float tx1 = tx0 + 1.f/(float)CX;
	float ty1 = ty0 + 1.f/(float)CY;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcDecoAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id <= 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "iconDeco" );
	float tx0 = 0;
	float ty0 = 0;
	float tx1 = 0;
	float ty1 = 0;

	if ( id != DECO_NONE ) {
		int y = id / 8;
		int x = id - y*8;
		tx0 = (float)x / 8.f;
		ty0 = (float)y / 4.f;
		tx1 = tx0 + 1.f/8.f;
		ty1 = ty0 + 1.f/4.f;
	}
	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_DECO : UIRenderer::RENDERSTATE_UI_DECO_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcPaletteAtom( int c0, int c1, int blend, bool enabled )
{
	Vector2I c = { 0, 0 };
	Texture* texture = TextureManager::Instance()->GetTexture( "palette" );

	static const int offset[5] = { 75, 125, 175, 225, 275 };
	static const int subOffset[3] = { 0, -12, 12 };

	if ( blend == PALETTE_NORM ) {
		if ( c1 > c0 )
			Swap( &c1, &c0 );
		c.x = offset[c0];
		c.y = offset[c1];
	}
	else {
		if ( c0 > c1 )
			Swap( &c0, &c1 );

		if ( c0 == c1 ) {
			// first column special:
			c.x = 25 + subOffset[blend];
			c.y = offset[c1];
		}
		else {
			c.x = offset[c0] + subOffset[blend];;
			c.y = offset[c1];
		}
	}
	RenderAtom atom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, 0, 0, 0, 0 );
	UIRenderer::SetAtomCoordFromPixel( c.x, c.y, c.x, c.y, 300, 300, &atom );
	return atom;
}


/*
	640x480 mininum screen.
	6 buttons
	80 pixels / per
*/
gamui::LayoutCalculator LumosGame::DefaultLayout()
{
	const Screenport& port = GetScreenport();
	LayoutCalculator layout( port.UIWidth(), port.UIHeight() );
	layout.SetGutter( 10.0f );
	layout.SetSize( 75.0f, 75.0f );
	layout.SetSpacing( 5.0f );
	return layout;
}


void LumosGame::InitStd( gamui::Gamui* g, gamui::PushButton* okay, gamui::PushButton* cancel )
{
	const ButtonLook& stdBL = GetButtonLook( LumosGame::BUTTON_LOOK_STD );
	gamui::LayoutCalculator layout = DefaultLayout();

	if ( okay ) {
		okay->Init( g, stdBL );
		okay->SetSize( layout.Width(), layout.Height() );
		okay->SetDeco( CalcDecoAtom( DECO_OKAY, true ), CalcDecoAtom( DECO_OKAY, false ) );
	}
	if ( cancel ) {
		cancel->Init( g, stdBL );
		cancel->SetSize( layout.Width(), layout.Height() );
		cancel->SetDeco( CalcDecoAtom( DECO_CANCEL, true ), CalcDecoAtom( DECO_CANCEL, false ) );
	}
}


void LumosGame::PositionStd( gamui::PushButton* okay, gamui::PushButton* cancel )
{
	gamui::LayoutCalculator layout = DefaultLayout();

	if ( okay )
		layout.PosAbs( okay, LumosGame::OKAY_X, -1 );
	
	if ( cancel )
		layout.PosAbs( cancel, LumosGame::CANCEL_X, -1 );
}


